// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include "user_input.h"

int main(int argc, char** argv)
{
    bool disable_fadeout = false;
    const char* path = "audio_data";
    process_cmdline_args(argc, argv, &path, &disable_fadeout);

    audio_file_container audioFiles;
    if (!load_files(audioFiles, path)) {
        puts("Error loading files...exiting.");
        return -1;
    }

    play_params p_params[2];
    paData data = paData(&audioFiles);
    get_user_params(p_params, audioFiles, disable_fadeout);
    data.p_params = p_params;

    pa_player audio_player;
    PaError err = audio_player.init_pa(&data);
    if (err != paNoError)
        return -1;

    run_user_loop(audioFiles, data, p_params, disable_fadeout);

    audio_player.deinit_pa();
    if (err != paNoError)
        return -1;

    return 0;
}
