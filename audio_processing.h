#pragma once
#include "AudioFile.h"

#define PI_DIV_4 0.78539816339f

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
}