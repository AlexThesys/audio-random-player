#pragma once

#include "audio_playback.h"

void get_user_params(play_params *data, const audio_file_container &audioFiles, bool no_fadeout);
bool load_files(audio_file_container &audioFiles, const char *dirpath);
void process_cmdline_args(int argc, char **argv, const char **res, bool *nofadeout);

inline void run_user_loop(const audio_file_container &audioFiles, paData &data, play_params *p_params,
                          bool disable_fadeout)
{
    int selector = 0;
    do {
        puts("\nEnter \'q\' to stop the playback or '\p'\ to change parameters...");
        const char ch = getchar();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'p' || ch == 'P') {
            selector ^= 1;
            get_user_params(&p_params[selector], audioFiles, disable_fadeout);
            data.p_params = &p_params[selector];
        } else {
            while ((getchar()) != '\n'); // flush stdin
        }
    } while (true);
}