// same as in constants.h
#define FRAMES_PER_BUFFER 0x100
#define NUM_CHANNELS 2
#define VIZ_BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS)

#define s16_min 32768.0f
#define s16_max 32767.0f
#define conversion_factor (2.0f / (s16_max + s16_min))

float s16_to_float(short val)
{
	return ((float(val) + s16_min) * conversion_factor) - 1.0f;
}

__kernel void compute_fft(int* input, float2* output, int num_bands, int fp_mode) {
	const int t_id = get_global_id(0);
	const float wf_input = (!fp_mode) ? s16_to_float(((__global short*)input)[t_id]) : as_float(input[t_id]);
	
	const float2 fft_output = {wf_input, 1.0f};
	output[t_id] = fft_output;
}
