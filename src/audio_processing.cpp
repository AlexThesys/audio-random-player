#include "audio_processing.h"
#include "profiling.h"

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

    PROFILE_START("resample");

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

    PROFILE_STOP("resample");

    return frames_read;
}

void apply_volume(buffer_container &buffer, size_t num_channels, float volume, bool use_lfo,
                  dsp::modulation::lfo &lfo_gen)
{
    if (!use_lfo) {
        const __m128 vol = _mm_set1_ps(volume);
        for (size_t ch = 0; ch < num_channels; ch++) {
            for (__m128 &m : buffer[ch]) {
                m = _mm_mul_ps(m, vol);
            }
        }
    } else {
        for (size_t ch = 0; ch < num_channels; ch++) {
            for (__m128& m : buffer[ch]) {
                alignas(16) float lfo[FP_IN_VEC];
                lfo[0] = lfo_gen.update(ch);
                lfo[1] = lfo_gen.update(ch);
                lfo[2] = lfo_gen.update(ch);
                lfo[3] = lfo_gen.update(ch);
                m = _mm_mul_ps(m, *(__m128*)lfo);
            }
        }
    }
}

int calculate_note_frames(int bpm, int note_length_divisor, const size_t max_lenght_samples, bool no_fadeout)
{
    int note_frames;
    if (no_fadeout) { // always play full length of all the files - don't fade out them
        note_frames = (int)max_lenght_samples;
    }
    else {
        constexpr float min2sec = 1.0f / 60.0f;
        const float bps = (float)bpm * min2sec;
        const float spb = 1.0f / bps;
        const float beat_lenght = 4.0f; // because bpm acually means 1/4th per second
        const float note_len_sec = spb * (beat_lenght / (float)note_length_divisor);
        note_frames = static_cast<int>(ceilf(static_cast<float>(SAMPLE_RATE) * note_len_sec));
    }
    return note_frames;
}