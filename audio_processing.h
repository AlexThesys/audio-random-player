#pragma once
#include "AudioFile.h"
#include "xmmintrin.h"

#define kPi 3.14159265359 
#define PI_DIV_4 0.78539816339f
#define LFO_BUFFER_SIZE 1024

static int resample(const AudioFile<float>::AudioBuffer &source, std::array<std::vector<float>, 2> &dest,
					int file_offset, int in_samples, int out_samples, int frames_per_buffer, int num_ch, bool fadeout) {

	constexpr int fade_prefered_lenght = 40;

	int i;
	int frames_read = in_samples;
	if (in_samples == out_samples) {
		// common case - just copy and attenuate
		for (int ch = 0; ch < num_ch; ch++) {
			i = 0;
			for (int j = file_offset; i < in_samples; i++, j++) {
				dest[ch][i] = source[ch][j];
			}
			// if it's the last one - pad with zeros
			if (out_samples < frames_per_buffer) {
				if (fadeout) {
					const int fade_out = (out_samples < fade_prefered_lenght) ? out_samples : fade_prefered_lenght;
					const float decrement = 1.0f / (float)fade_out;
					float factor = 1.0f;
					for (i = out_samples - fade_out; i < out_samples; i++) {
						dest[ch][i] *= factor;
						factor -= decrement;
					}
				}
				for (; i < frames_per_buffer; i++) {
					dest[ch][i] = 0.0f;
				}
			}
		}
	} else {
		const float d = float(in_samples) / float(out_samples);
		for (int ch = 0; ch < num_ch; ch++) {
			dest[ch][0] = source[ch][file_offset];
			i = 1;
			for (int j = file_offset + 1, sz = source[ch].size(); i < out_samples; i++) {
				const float x = float(i) * d;
				const int  y = int(x);
				const float z = x - float(y);
				assert(y < in_samples);
				const int idx = j + y;
				if ((idx + 1) >= sz)
					break;
				const float res = source[ch][idx] * (1.0f - z) + source[ch][idx + 1] * z;
				dest[ch][i] = res;
			}
			//if it's the last one - pad with zeros
			if (out_samples < frames_per_buffer) {
				if (fadeout) {
					const int fade_out = (out_samples < fade_prefered_lenght) ? out_samples : fade_prefered_lenght;
					const float decrement = 1.0f / (float)fade_out;
					float factor = 1.0f;
					for (i = out_samples - fade_out; i < out_samples; i++) {
						dest[ch][i] *= factor;
						factor -= decrement;
					}
				}
				for (; i < frames_per_buffer; i++) {
					dest[ch][i] = 0.0f;
				}
				if ((out_samples < frames_per_buffer) && (d < 1.0f))
					frames_read = out_samples;
			}
		}
	}
	return frames_read;
}

namespace dsp {
namespace waveshaper {
	struct params {
		float coef_pos;
		float coef_neg;
		int32_t num_stages;
		int32_t invert_stages;
		float gain;
	};

	static const params default_params = { 0.2f, 1.8f, 9, 1, 0.12f };

	typedef union fp32_to_u32 {
		float			f;
		uint32_t		u;
	}					fp32_to_u32;

	inline float fast_atan(float x) {
		fp32_to_u32		f2u;
		f2u.f = x;
		f2u.u &= 0x7FFFFFFF;
		return			PI_DIV_4 * x - x * (f2u.f - 1.0f) * (0.2447f + 0.0663f * f2u.f);

		//const int abs_x_bits = *(uint32_t*)&x & 0x7FFFFFFF;
		//const float abs_x = *(float*)&abs_x_bits;
		//return M_PI_4 * x - x * (abs_x - 1.0f) * (0.2447f + 0.0663f * abs_x);
	}

	void process		(std::array<std::vector<float>, 2>& buffer, const params& p) {
		for (uint32_t ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			for (uint32_t i = 0, frames = buffer[ch].size(); i < frames; i++) {
				float sample		= buffer[ch][i];
				for (uint32_t j = 0; j < p.num_stages; j++) {
					const int32_t mask = *(int32_t*)&sample >> 0x1f;
					fp32_to_u32	coeff;
					coeff.u		= (~mask & *(uint32_t*)&p.coef_pos) | (mask & *(uint32_t*)&p.coef_neg);
					sample		= (1.0f / fast_atan(coeff.f)) * fast_atan(coeff.f * sample);
					const uint32_t inverted = *(uint32_t*)&sample ^ (0x80000000 & ~((p.invert_stages & j) - 0x01));
					sample		= *(float*)&inverted;
				}
				sample			*= p.gain;
				buffer[ch][i]	= sample;
			}
		}
	}
}

// v1
//inline float	hadd_sse(const __m128 &accum) {
//	__m128 lo								= _mm_unpacklo_ps(accum, accum);
//	__m128 hi								= _mm_unpackhi_ps(accum, accum);
//	lo										= _mm_add_ps(lo, hi);
//	hi										= _mm_unpackhi_ps(hi, hi);
//	lo										= _mm_add_ss(lo, hi);
//	return									_mm_cvtss_f32(lo);
//}
// v2
inline float	hadd_sse(const __m128& accum) {
	__m128 shuf = _mm_shuffle_ps(accum, accum, _MM_SHUFFLE(2, 3, 0, 1));
	__m128 sums = _mm_add_ps(accum, shuf);
	shuf = _mm_movehl_ps(shuf, sums);
	sums = _mm_add_ss(sums, shuf);
	return _mm_cvtss_f32(sums);
}

typedef float CalcT;

namespace biquad {
	struct vec {
		union {
			__m128 m;
			alignas(16) float data[4];
		};
		CalcT _dot(vec other) {
			return this->data[0] * other.data[0] + this->data[1] * other.data[1] 
					+ this->data[2] * other.data[2] + this->data[3] * other.data[3];
		}
		CalcT _dot4(vec other) {
			other.m = _mm_mul_ps(this->m, other.m);
			return hadd_sse(other.m);
		}
	};

	template <int num_channels>
	struct filter_base
	{
		vec		b12a01;
		vec h0123[num_channels];
		float	padding;
	public:
		filter_base() {
			clear();
		}
		inline void	clear() {
			memset(h0123, 0, sizeof(h0123));
		}
		inline void	setup(CalcT a0, CalcT a1, CalcT a2, CalcT b0, CalcT b1, CalcT b2)
		{
			CalcT inv_a0 = 1.f / a0;

			b12a01.data[2] = -a1 * inv_a0;
			b12a01.data[3] = -a2 * inv_a0;
			padding = b0 * inv_a0;
			b12a01.data[0] = b1 * inv_a0;
			b12a01.data[1] = b2 * inv_a0;
		}

		inline CalcT processI(CalcT in, int ch = 0)
		{
			assert(ch < num_channels);
			if (0) {
				CalcT out = padding * in + b12a01.data[0] * h0123[ch].data[0] + b12a01.data[1] * h0123[ch].data[1] 
					+ b12a01.data[2] * h0123[ch].data[2] + b12a01.data[3] * h0123[ch].data[3];
				h0123[ch].data[1] = h0123[ch].data[0]; h0123[ch].data[0] = in;
				h0123[ch].data[3] = h0123[ch].data[2]; h0123[ch].data[2] = out;
				return			out;
			}
			else {

				float	out = padding * in + b12a01._dot4(h0123[ch]);

				__m128	io = _mm_unpacklo_ps(_mm_set_ss(in), _mm_set_ss(out));
				__m128	h0h2 = _mm_shuffle_ps(h0123[ch].m, h0123[ch].m, _MM_SHUFFLE(2, 0, 2, 0));
				h0123[ch].m = _mm_unpacklo_ps(io, h0h2);

				return out;
			}
		}

		inline void	process(size_t frames, float* dest, int ch = 0) {
			assert(ch < num_channels);
			while (frames--) {
				CalcT in = CalcT(*dest);
				in = processI(in, ch);
				*dest++ = float(in);
			}
		}
	};

//	Biquad Second Order IIR Low pass Filter
	template <int num_channels>
	struct low_pass : public filter_base<num_channels> {
		inline	void setup(CalcT normFreq, CalcT q) {
			CalcT w0 = 2 * kPi * normFreq;
			CalcT cs = cos(w0);
			CalcT sn = sin(w0);
			CalcT ncs = 1 - cs;

			CalcT alph = sn / (2 * q);
			CalcT b0 = ncs * CalcT(0.5);
			CalcT b1 = ncs;
			CalcT b2 = b0;
			CalcT a0 = 1 + alph;
			CalcT a1 = -2 * cs;
			CalcT a2 = 1 - alph;

			filter_base<num_channels>::setup(a0, a1, a2, b0, b1, b2);
		}
	};
}
class filter {
	biquad::low_pass<2> _lpf;
public:
	void setup(float f, float r) {
		_lpf.clear();
		_lpf.setup(f / (float)SAMPLE_RATE, r);
	}
	void process(std::array<std::vector<float>, 2>& buffer) {
		for (int ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			float* dest = buffer[ch].data();
			_lpf.process(buffer[ch].size(), dest, ch);
		}
	}

};

class wavetable {
	float _buffer[LFO_BUFFER_SIZE];
	float _read_idx[NUM_CHANNELS];
	float _inc;
	float _amount;

public:
	wavetable() : _inc(0.0f), _amount(0.0f) {
		for (int i = 0; i < LFO_BUFFER_SIZE; i++) {
			_buffer[i] = (1.0f - sinf(((float)i / (float)LFO_BUFFER_SIZE) * 2.0f * kPi)) * 0.5f;
		}
		memset(_read_idx, 0, sizeof(_read_idx));
	}
	void set_rate(float freq, float amount) {
		memset(_read_idx, 0, sizeof(_read_idx));
		_inc = (float)LFO_BUFFER_SIZE * freq / SAMPLE_RATE;
		_amount = amount;
	}
	float update(int ch) {
		int r_idx = (int)_read_idx[ch];
		float frac = _read_idx[ch] - (float)r_idx;
		const int r_idx_next = (r_idx + 1) & (LFO_BUFFER_SIZE - 1);
		const float res = _buffer[r_idx] * (1.0f - frac) + _buffer[r_idx_next] * frac;
		_read_idx[ch] += _inc;
		r_idx = (int)_read_idx[ch];
		frac = _read_idx[ch] - (float)r_idx;
		r_idx &= (LFO_BUFFER_SIZE - 1);
		_read_idx[ch] = (float)r_idx + frac;
		return  1.0f - (res * _amount);
	}
};
}

void apply_volume(std::array<std::vector<float>, 2>& buffer, float volume, bool use_lfo, dsp::wavetable &lfo_gen) {
	if (!use_lfo) {
		for (int ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			for (int i = 0, sz = buffer[ch].size(); i < sz; i++) {
				buffer[ch][i] *= volume;
			}
		}
	} else {
		for (int ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			for (int i = 0, sz = buffer[ch].size(); i < sz; i++) {
				buffer[ch][i] *= lfo_gen.update(ch);
			}
		}
	}
}