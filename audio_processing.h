#pragma once
#include "AudioFile.h"

static void resample(const AudioFile<float>::AudioBuffer &source, std::array<std::vector<float>, 2> &dest,
					int file_offset, int in_samples, int out_samples, int frames_per_buffer, float volume, int num_ch) {

	const float* __restrict	data[2];
	data[0] = source[0].data() + file_offset;
	if (num_ch == 2)
		data[1] = source[1].data() + file_offset;

	if (in_samples == out_samples) {
		// common case - just copy and attenuate
		for (int ch = 0; ch < num_ch; ch++) {
			int i;
			for (i = 0; i < in_samples; ++i) {
				dest[ch][i] = data[ch][i] * volume;
			}
			// if it's the last one - pad with zeros
			for (; i < frames_per_buffer; i++) {
				dest[ch][i] = 0.0f;
			}
		}
	} else {
		const float d = float(in_samples) / float(out_samples);
		for (int ch = 0; ch < num_ch; ch++) {
			dest[ch][0] = data[ch][0] * volume;
			for (int i = 1; i < out_samples; ++i) {
				const float x = float(i) * d;
				const int  y = int(x);
				const float z = x - float(y);
				assert(y < in_samples);
				const float res = data[ch][y] * (1.f - z) + data[ch][y + 1] * z;
				dest[ch][i] = res * volume;
			}
			// if it's the last one - pad with zeros
			for (int i = out_samples; i < frames_per_buffer; i++) {
				dest[ch][i] = 0.0f;
			}
		}
	}
}