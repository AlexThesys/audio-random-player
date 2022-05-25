#pragma once

#include "audio_playback.h"

void get_user_params(play_params *data, const audio_file_container &audioFiles, bool no_fadeout);
bool load_files(audio_file_container &audio_files, const char *dirpath);
void process_cmdline_args(int argc, char **argv, const char **res, bool *nofadeout);
void run_user_loop(const audio_file_container &audioFiles, pa_data &data, play_params *p_params, bool disable_fadeout);