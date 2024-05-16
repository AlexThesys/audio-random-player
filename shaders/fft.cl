// same as in constants.h
#define FRAMES_PER_BUFFER 0x100
#define NUM_CHANNELS 2
#define VIZ_BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS)

#define s16_min 32768.0f
#define s16_max 32767.0f
#define conversion_factor (2.0f / (s16_max + s16_min))

#define PI 3.14159265f

float s16_to_float(short val) {
	return (((float)val + s16_min) * conversion_factor) - 1.0f;
}

__kernel void compute_fft(__global int* input, __global float2* output, int fp_mode) {
	const int t_id = get_global_id(0);
	const int N = VIZ_BUFFER_SIZE;

	float2 sum = { 0.0f, 0.0f };

    const float c = -2.0f * PI * (float)t_id / (float)N;

	if (!fp_mode) {
		for (size_t n = t_id & 0x01; n < N; n += 2) {
			const int half_n = n / 2;
			const float cn = c * (float)(half_n);
			float2 t = { cos(cn), sin(cn) };
			const float xn = s16_to_float(((__global short*)input)[n]);
			const float2 xnv = { xn, -xn };
			t *= xnv;
			sum += t;
		}
	} else {
		for (size_t n = t_id & 0x01; n < N; n += 2) {
			const int half_n = n / 2;
			const float cn = c * (float)(half_n);
			float2 t = { cos(cn), sin(cn) };
			const float xn = as_float(input[n]);
			const float2 xnv = { xn, -xn };
			t *= xnv;
			sum += t;
		}
	}
	//sum.x = fabs(sum.x);
	output[t_id] = sum;
}
