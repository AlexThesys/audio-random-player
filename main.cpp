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

    play_params p_params[2];
    paData data = paData(audioFiles);
    int selector = 0;
    get_user_params(p_params, audioFiles);
    data.p_params = p_params;

    pa_player audio_player;
    PaError err = audio_player.init_pa(&data);
    if (err != paNoError)
        return -1;

    do {
        puts("\nEnter \'q\' to stop the playback or '\p'\ to change parameters...");
        const char ch = getchar();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'p' || ch == 'P') {
            selector ^= 1;
            get_user_params(&p_params[selector], audioFiles);
            data.p_params = &p_params[selector];
        }
    } while (true);

    audio_player.deinit_pa();
    if (err != paNoError)
        return -1;

    return 0;
}
