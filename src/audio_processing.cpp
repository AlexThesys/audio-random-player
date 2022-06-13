#include "audio_processing.h"

inline void apply_fadeout(float *dest, size_t out_samples)
{
    constexpr int fade_prefered_lenght = 40;
    const int count = static_cast<int>(out_samples);
    const int fade_out = (count < fade_prefered_lenght) ? count : fade_prefered_lenght;
    const float decrement = 1.0f / static_cast<float>(fade_out);
    float factor = 1.0f;
    for (size_t i = out_samples - static_cast<size_t>(fade_out); i < out_samples; i++) {
        dest[i] *= factor;
        factor -= decrement;
    }
}

size_t resample(const AudioFile<float>::AudioBuffer &source, buffer_container &destination, size_t file_offset, size_t in_samples,
             size_t out_samples, size_t frames_per_buffer, size_t num_ch, bool fadeout)
{
    float *dest[NUM_CHANNELS];
    dest[0] = (float *)destination[0].data();
    dest[1] = (float *)destination[1].data();
    size_t frames_read = in_samples;
    if (in_samples == out_samples) {
        // common case - just copy and attenuate
        for (size_t ch = 0; ch < num_ch; ch++) {
            memcpy(dest[ch], (const void *)(source[ch].data() + file_offset), sizeof(float) * out_samples);
            // if it's the last one - pad with zeros
            if (out_samples < frames_per_buffer) {
                if (fadeout)
                    apply_fadeout(dest[ch], out_samples);
                memset(dest[ch], 0, sizeof(float) * (frames_per_buffer - out_samples));
            }
        }
    } else {
        const float d = float(in_samples) / float(out_samples);
        for (size_t ch = 0; ch < num_ch; ch++) {
            dest[ch][0] = source[ch][file_offset];
            for (size_t i = 1, j = file_offset + 1, sz = source[ch].size(); i < out_samples; i++) {
                const float x = float(i) * d;
                const int y = int(x);
                const float z = x - float(y);
                assert(static_cast<size_t>(y) < in_samples);
                const size_t idx = j + static_cast<size_t>(y);
                if ((idx + 1) >= sz)
                    break;
                const float res = source[ch][idx] * (1.0f - z) + source[ch][idx + 1] * z;
                dest[ch][i] = res;
            }
            // if it's the last one - pad with zeros
            if (out_samples < frames_per_buffer) {
                if (fadeout)
                    apply_fadeout(dest[ch], out_samples);
                memset(dest[ch], 0, sizeof(float) * (frames_per_buffer - out_samples));
                if ((out_samples < frames_per_buffer) && (d < 1.0f))
                    frames_read = out_samples;
            }
        }
    }
    return frames_read;
}

void apply_volume(buffer_container &buffer, float volume, bool use_lfo, dsp::wavetable &lfo_gen)
{
    if (!use_lfo) {
        const __m128 vol = _mm_set1_ps(volume);
        for (std::vector<__m128> &b : buffer) {
            for (__m128 &m : b) {
                m = _mm_mul_ps(m, vol);
            }
        }
    } else {
        for (size_t ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
            for (__m128& m : buffer[ch]) {
                alignas(16) float lfo[F_IN_VEC];
                lfo[0] = lfo_gen.update(ch);
                lfo[1] = lfo_gen.update(ch);
                lfo[2] = lfo_gen.update(ch);
                lfo[3] = lfo_gen.update(ch);
                m = _mm_mul_ps(m, *(__m128*)lfo);
            }
        }
    }
}