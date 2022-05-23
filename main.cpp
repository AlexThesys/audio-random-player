// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>

#include "audio_playback.h"

int main()
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

    paData data = { audioFiles, {0, 0, 0, 0}, 0, 0};

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
