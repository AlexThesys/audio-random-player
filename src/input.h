#pragma once

#include "audio_playback.h"

#define NUM_FILES 4
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

int calculate_step_time(int walk_speed, const std::vector<AudioFile<float>>& audioFiles, bool no_fadeout) {
    const float step_size = 0.762f; // for an average man
    const float kph_2_mps_rec = 1.0f / 3.6f;
    const float w_speed = ((float)walk_speed) * kph_2_mps_rec;
    int step_num_frames = ceilf((float)SAMPLE_RATE * step_size / w_speed);
    if (no_fadeout) {   // always play full length of all the files - don't fade out them
        for (int i = 0, sz = audioFiles.size(); i < sz; i++) {
            step_num_frames = _max(step_num_frames, audioFiles[i].getNumSamplesPerChannel());
        }
    }
    return step_num_frames;
}

inline float calculate_volume_lower_bound(float volume_deviation) {
    return powf(10, -0.05f * volume_deviation);
}

int get_input(int default_value) {
    char line[4];
    memset(line, 0, sizeof(line));
    scanf_s("%s", line, (unsigned int)_countof(line));
    char* end;
    long l = strtol((const char*)line, &end, (int)10);
    if (end == line || end[0] != '\0') {
        l = default_value;
    }
    return (int)l;
}
template <typename T>
int clamp_input(T val, T min, T max) {
    val = _min(val, max);
    return _max(val, min);
}

void get_user_params(play_params* data, const std::vector<AudioFile<float>>& audioFiles, bool no_fadeout) {
	puts("Please provide the playback parameters in decimal integer format!");
	puts("Enter any letter to skip the parameter and use default.");
    printf("\nEnter the walk speed in kph [1...12]:\t");
    const int walk_speed = clamp_input(get_input(DEFAULT_WALK_SPEED), MIN_WALK_SPEED, MAX_WALK_SPEED);
    printf("%d\n", walk_speed);
    const int step_num_frames = calculate_step_time(walk_speed, audioFiles, no_fadeout);
    printf("\nEnter the pitch deviation in semitones [0...12]:\t");
    const int pitch_dev = clamp_input(get_input(DEFAULT_PITCH_DEVIATION), MIN_PITCH_DEVIATION, MAX_PITCH_DEVIATION);
    printf("%d\n", pitch_dev);
    const float pitch_deviation = (float)pitch_dev;
    printf("\nEnter the volume deviation in dB [0...90]:\t");
    const int volume_lb = clamp_input(get_input(DEFAULT_VOLUME_LOWER_BOUND), MIN_VOLUME_LOWER_BOUND, MAX_VOLUME_LOWER_BOUND);
    printf("%d\n", volume_lb);
    const float volume_lower_bound = calculate_volume_lower_bound((float)volume_lb);   
    printf("\nEnter the LPF frequency deviation in KHZ [0...19]:\t");
    const int lpf_f = clamp_input(get_input(DEFAULT_LPF_FREQ_DEVIATION), MIN_LPF_FREQ_DEVIATION, MAX_LPF_FREQ_DEVIATION);
    printf("%d\n", lpf_f);
    const float lpf_freq = (float)lpf_f * 1000.0f;
    printf("\nEnter the LPF Q deviation [0...8]:\t");
    const int lpf_q_i = clamp_input(get_input(DEFAULT_LPF_Q_DEVIATION), MIN_LPF_Q_DEVIATION, MAX_LPF_Q_DEVIATION);
    printf("%d\n", lpf_q_i);
    const float lpf_q = (float)lpf_q_i;

    puts("\nEnter non-zero value to use LFO for volume modulation.");
    printf("Enter LFO modulation frequency in Hz [1...10]:\t");
    int lfo_f = get_input(DEFAULT_LFO_FREQ);
    bool use_lfo = !!lfo_f;
    float lfo_freq = 0.0f;
    float lfo_amount = 0.0f;  
    if (use_lfo) {
        lfo_f = clamp_input(lfo_f, MIN_LFO_FREQ, MAX_LFO_FREQ);
        printf("%d\n", lfo_f);
        lfo_freq = float(lfo_f);
        printf("\nEnter LFO modulation amount [0...100]:\t");
        const int lfo_a = clamp_input(get_input(DEFAULT_LFO_AMOUNT), MIN_LFO_AMOUNT, MAX_LFO_AMOUNT);
        printf("%d\n", lfo_a);
        lfo_amount = (float)lfo_a / (float)MAX_LFO_AMOUNT;
    }

    while ((getchar()) != '\n');    // flush stdin

    printf("\nEnable distortion? [y/n]\t");
    char ch = getchar();
    const bool enable_dist = (ch == 'y' || ch == 'Y');
    printf("%c\n", enable_dist ? 'y' : 'n');

    while ((getchar()) != '\n');    // flush stdin

    data->init(step_num_frames, pitch_deviation, volume_lower_bound, lpf_freq, lpf_q, lfo_freq, lfo_amount, use_lfo, enable_dist);
}