#pragma once

#include <array>

#include "AudioFile.h"
#include "constants.h"
#include "utils.h"
#include "xmmintrin.h"

namespace dsp
{
namespace waveshaper
{
struct params
{
    float coef_pos;
    float coef_neg;
    uint32_t num_stages;
    uint32_t invert_stages;
    float gain;
};

void process(buffer_container &buffer, size_t num_channels, const params &p);

static const params default_params = {0.2f, 1.8f, 9, 1, 0.12f};
} // namespace waveshaper

// v1
// inline float	hadd_sse(const __m128 &accum) {
//	__m128 lo								= _mm_unpacklo_ps(accum, accum);
//	__m128 hi								= _mm_unpackhi_ps(accum, accum);
//	lo										= _mm_add_ps(lo, hi);
//	hi										= _mm_unpackhi_ps(hi, hi);
//	lo										= _mm_add_ss(lo, hi);
//	return									_mm_cvtss_f32(lo);
//}
// v2
inline float hadd_sse(const __m128 &accum)
{
    __m128 shuf = _mm_shuffle_ps(accum, accum, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(accum, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

typedef float calc_t;

namespace biquad
{
struct vec
{
    union {
        __m128 m;
        alignas(16) float data[FP_IN_VEC];
    };
    calc_t _dot(vec other)
    {
        return this->data[0] * other.data[0] + this->data[1] * other.data[1] + this->data[2] * other.data[2] +
               this->data[3] * other.data[3];
    }
    calc_t _dot4(vec other)
    {
        other.m = _mm_mul_ps(this->m, other.m);
        return hadd_sse(other.m);
    }
};

//	Biquad Second Order IIR Low pass Filter
template <int num_channels> 
struct low_pass
{
    vec b12a01;
    vec h0123[num_channels];
    float pad;
    // padding 12 bytes

  public:
    low_pass()
    {
        clear();
    }
    inline void clear()
    {
        memset(h0123, 0, sizeof(h0123));
    }
    inline void setup(calc_t normFreq, calc_t q)
    {
        calc_t w0 = calc_t(2.0) * PI * normFreq;
        calc_t cs = cos(w0);
        calc_t sn = sin(w0);
        calc_t ncs = calc_t(1.0) - cs;

        calc_t alph = sn / (calc_t(2.0) * q);
        calc_t b0 = ncs * calc_t(0.5);
        calc_t b1 = ncs;
        calc_t b2 = b0;
        calc_t a0 = calc_t(1.0) + alph;
        calc_t a1 = -calc_t(2.0) * cs;
        calc_t a2 = calc_t(1.0) - alph;

        setup(a0, a1, a2, b0, b1, b2);
    }
    inline void process(size_t frames, float *dest, int ch = 0)
    {
        assert(ch < num_channels);
        while (frames--) {
            calc_t in = calc_t(*dest);
            in = process(in, ch);
            *dest++ = float(in);
        }
    }

private:
    inline void setup(calc_t a0, calc_t a1, calc_t a2, calc_t b0, calc_t b1, calc_t b2)
    {
        calc_t inv_a0 = 1.f / a0;

        b12a01.data[2] = -a1 * inv_a0;
        b12a01.data[3] = -a2 * inv_a0;
        pad = b0 * inv_a0;
        b12a01.data[0] = b1 * inv_a0;
        b12a01.data[1] = b2 * inv_a0;
    }
    inline calc_t process(calc_t in, int ch = 0)
    {
        assert(ch < num_channels);
        float out = pad * in + b12a01._dot4(h0123[ch]);

        __m128 io = _mm_unpacklo_ps(_mm_set_ss(in), _mm_set_ss(out));
        __m128 h0h2 = _mm_shuffle_ps(h0123[ch].m, h0123[ch].m, _MM_SHUFFLE(2, 0, 2, 0));
        h0123[ch].m = _mm_unpacklo_ps(io, h0h2);

        return out;
    }
};
} // namespace biquad

class filter
{
    biquad::low_pass<NUM_CHANNELS> _lpf;

  public:
    void setup(float f, float r)
    {
        _lpf.clear();
        _lpf.setup(f / (float)SAMPLE_RATE, r);
    }
    void process(buffer_container &buffer, size_t num_channels)
    {
        const size_t b_size = buffer[0].size() * FP_IN_VEC;
        for (size_t ch = 0; ch < num_channels; ch++) {
            float *dest = (float*)buffer[ch].data();
            _lpf.process(b_size, dest, (int)ch);
        }
    }
};

namespace modulation {
    class wavetable {
        float _buffer[LFO_BUFFER_SIZE];
    public:
        wavetable()
        {
            for (int i = 0; i < LFO_BUFFER_SIZE; i++) {
                _buffer[i] =
                    (1.0f - sinf((static_cast<float>(i) / static_cast<float>(LFO_BUFFER_SIZE)) * 2.0f * PI)) * 0.5f;
            }
        }
        const float *buffer() const
        {
            return _buffer;
        }
    };

    class lfo
    {
        const wavetable *_wt;
        float _read_idx[NUM_CHANNELS];
        float _inc;
        float _amount;

    public:
        lfo(const wavetable* wt) : _wt(wt),  _inc(0.0f), _amount(0.0f)
        {
            memset(_read_idx, 0, sizeof(_read_idx));
        }
        void set_rate(float freq, float amount)
        {
            memset(_read_idx, 0, sizeof(_read_idx));
            _inc = static_cast<float>(LFO_BUFFER_SIZE) * freq / static_cast<float>(SAMPLE_RATE);
            _amount = amount;
        }
        float update(size_t ch)
        {
            int r_idx = static_cast<int>(_read_idx[ch]);
            float frac = _read_idx[ch] - static_cast<float>(r_idx);
            const int r_idx_next = (r_idx + 1) & (LFO_BUFFER_SIZE - 1);
            const float res = _wt->buffer()[r_idx] * (1.0f - frac) + _wt->buffer()[r_idx_next] * frac;
            _read_idx[ch] += _inc;
            r_idx = static_cast<int>(_read_idx[ch]);
            frac = _read_idx[ch] - (float)r_idx;
            r_idx &= (LFO_BUFFER_SIZE - 1);
            _read_idx[ch] = static_cast<float>(r_idx) + frac;
            return 1.0f - (res * _amount);
        }
    };
}
} // namespace dsp