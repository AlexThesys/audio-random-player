// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>

#include "audio_playback.h"

#define MAX_PITCH_DEVIATION 12

int calculate_step_time(int walk_speed, const AudioFile<float> *audioFiles) {
    const float step_size = 0.762f; // for an average man
    const float kph_2_mps_rec = 1.0f / 3.6f;
    const float w_speed = (float)walk_speed * kph_2_mps_rec;
    int step_num_frames = ceilf((float)SAMPLE_RATE * step_size / w_speed);
    step_num_frames = ((step_num_frames - 1) | (4 - 1)) + 1;  // make multiple of 4
    for (int i = 0; i < NUM_FILES; i++) {
        step_num_frames = _max(step_num_frames, audioFiles[i].getNumSamplesPerChannel());
    }
    return step_num_frames;
}

inline float calculate_volume_lower_bound(float volume_deviation) {
    return powf(10, -0.05f * volume_deviation);
}

int main(int argc, char** argv)
{
    AudioFile<float> audioFiles[NUM_FILES];
    audioFiles[0].load("audio_data/footsteps_grass_run_01.wav");
    audioFiles[0].setAudioBufferSize(audioFiles[0].getNumChannels(),
                                    ((audioFiles[0].getNumSamplesPerChannel() - 1) | (4 - 1)) + 1);  // make multiple of 4
    audioFiles[1].load("audio_data/footsteps_grass_run_02.wav");
    audioFiles[1].setAudioBufferSize(audioFiles[1].getNumChannels(),
                                    ((audioFiles[1].getNumSamplesPerChannel() - 1) | (4 - 1)) + 1);
    audioFiles[2].load("audio_data/footsteps_grass_run_03.wav");
    audioFiles[2].setAudioBufferSize(audioFiles[2].getNumChannels(),
                                    ((audioFiles[2].getNumSamplesPerChannel() - 1) | (4 - 1)) + 1);
    audioFiles[3].load("audio_data/footsteps_grass_run_04.wav");
    audioFiles[3].setAudioBufferSize(audioFiles[3].getNumChannels(),
                                    ((audioFiles[3].getNumSamplesPerChannel() - 1) | (4 - 1)) + 1);
    const int step_num_frames = calculate_step_time(6, audioFiles);
    const float volume_lower_bound = calculate_volume_lower_bound((float)6);

    paData data = paData(audioFiles, step_num_frames, (float)_min(8, MAX_PITCH_DEVIATION), volume_lower_bound);

    pa_player audio_player;
    PaError err = audio_player.init_pa(&data);
    if (err != paNoError)
        return -1;

    puts("Press Enter to stop the playback.");
    const char ch = getchar();

    audio_player.deinit_pa();
    if (err != paNoError)
        return -1;

    return 0;
}