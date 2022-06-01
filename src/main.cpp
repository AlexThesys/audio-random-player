// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>

#include "user_input.h"
#include <stdio.h>

volatile  play_params *middle_buf;
volatile LONG new_data;

int main(int argc, char **argv)
{
    bool disable_fadeout = false;
    const char *path = "audio_data";
    process_cmdline_args(argc, argv, &path, &disable_fadeout);

    audio_file_container audioFiles;
    if (!load_files(audioFiles, path)) {
        puts("Error loading files...exiting.");
        return -1;
    }

    play_params p_params[3];
    pa_data data(&audioFiles);
    get_user_params(&p_params[1], audioFiles, disable_fadeout);
    data.front_buf = &p_params[2];
    middle_buf = &p_params[1];
    new_data = 1;

    pa_player audio_player;
    if (audio_player.init_pa(&data) != paNoError)
        return -1;

    run_user_loop(audioFiles, data, p_params, disable_fadeout);

    if (audio_player.deinit_pa() != paNoError)
        return -1;

    return 0;
}
