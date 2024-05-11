// same as in constants.h
#define FRAMES_PER_BUFFER 0x100
#define NUM_CHANNELS 2
#define VIZ_BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS)

__kernel void compute_fft(int* input, float* output, int num_bands, int fp_mode) {

}
