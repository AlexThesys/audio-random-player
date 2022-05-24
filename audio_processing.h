#pragma once
#include "AudioFile.h"

static int resample(const AudioFile<float>::AudioBuffer &source, std::array<std::vector<float>, 2> &dest,
					int file_offset, int in_samples, int out_samples, int frames_per_buffer, float volume, int num_ch) {

	int i;
	int frames_read = in_samples;
	if (in_samples == out_samples) {
		// common case - just copy and attenuate
		for (int ch = 0; ch < num_ch; ch++) {
			i = 0;
			for (int j = file_offset; i < in_samples; i++, j++) {
				dest[ch][i] = source[ch][j] * volume;
			}
			// if it's the last one - pad with zeros
			for (; i < frames_per_buffer; i++) {
				dest[ch][i] = 0.0f;
			}
		}
	} else {
		const float d = float(in_samples) / float(out_samples);
		for (int ch = 0; ch < num_ch; ch++) {
			dest[ch][0] = source[ch][file_offset] * volume;
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
				dest[ch][i] = res * volume;
			}
			//if it's the last one - pad with zeros
			//if (out_samples < frames_per_buffer) {
			//	constexpr int prefered_lenght = 40;
			//	const int lerp_lenght = (out_samples < prefered_lenght) ? out_samples : prefered_lenght;
			//	const float decrement = 1.0f / (float)lerp_lenght;
			//	float factor = 1.0f;
			//	for (i = out_samples - lerp_lenght; i < out_samples; i++) {
			//		dest[ch][i] *= factor;
			//		factor -= decrement;
			//	}
				for (; i < frames_per_buffer; i++) {
					dest[ch][i] = 0.0f;
				}
				if ((out_samples < frames_per_buffer) && (d < 1.0f))
					frames_read = out_samples;
			//}
		}
	}
	return frames_read;
}