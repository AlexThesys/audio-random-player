#include "audio_processing.h"

int resample(const AudioFile<float>::AudioBuffer& source, buffer_container& dest,
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
	}
	else {
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

void apply_volume(buffer_container& buffer, float volume, bool use_lfo, dsp::wavetable& lfo_gen) {
	if (!use_lfo) {
		for (int ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			for (int i = 0, sz = buffer[ch].size(); i < sz; i++) {
				buffer[ch][i] *= volume;
			}
		}
	}
	else {
		for (int ch = 0, num_ch = buffer.size(); ch < num_ch; ch++) {
			for (int i = 0, sz = buffer[ch].size(); i < sz; i++) {
				buffer[ch][i] *= lfo_gen.update(ch);
			}
		}
	}
}