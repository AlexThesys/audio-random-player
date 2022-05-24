// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>

#include "input.h"


int main(int argc, char** argv)
{
    AudioFile<float> audioFiles[NUM_FILES];
    audioFiles[0].load("audio_data/footsteps_grass_run_01.wav");
    audioFiles[1].load("audio_data/footsteps_grass_run_02.wav");
    audioFiles[2].load("audio_data/footsteps_grass_run_03.wav");
    audioFiles[3].load("audio_data/footsteps_grass_run_04.wav");


    paData data;
    get_user_params(&data, audioFiles);

    pa_player audio_player;
    PaError err = audio_player.init_pa(&data);
    if (err != paNoError)
        return -1;

    puts("\nPress \'ENTER\' to stop the playback...");
    const char ch = getchar();

    audio_player.deinit_pa();
    if (err != paNoError)
        return -1;

    return 0;
}
