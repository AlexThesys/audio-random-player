#pragma once

#define SAMPLE_RATE (48000)
#define NUM_CHANNELS 2
#define NUM_FILES 4
#define FRAMES_PER_BUFFER 256
#define MAX_VOLUME 1.0f

#define PI 3.14159265359f
#define PI_DIV_4 0.78539816339f

#define MAX_DATA_SIZE 0x100000
#define LFO_BUFFER_SIZE 1024

#define MAX_LPF_FREQ 20000.0f
#define DEFAULT_LPF_Q 0.707f

#define DEFAULT_PITCH_DEVIATION 0
#define MAX_PITCH_DEVIATION 12
#define MIN_PITCH_DEVIATION 0
#define DEFAULT_VOLUME_LOWER_BOUND 0
#define MIN_VOLUME_LOWER_BOUND 0
#define MAX_VOLUME_LOWER_BOUND 90
#define DEFAULT_WALK_SPEED 4
#define MAX_WALK_SPEED 12
#define MIN_WALK_SPEED 1
#define DEFAULT_LPF_FREQ_DEVIATION 0
#define MIN_LPF_FREQ_DEVIATION 0
#define MAX_LPF_FREQ_DEVIATION 19
#define DEFAULT_LPF_Q_DEVIATION 0
#define MIN_LPF_Q_DEVIATION 0
#define MAX_LPF_Q_DEVIATION 8
#define DEFAULT_LFO_FREQ 0
#define MIN_LFO_FREQ 1
#define MAX_LFO_FREQ 20
#define DEFAULT_LFO_AMOUNT 100
#define MIN_LFO_AMOUNT 0
#define MAX_LFO_AMOUNT 100