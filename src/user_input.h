#pragma once

#include "audio_playback.h"

struct user_params {
	size_t max_lenght_samples = 0;
	bool disable_fadeout = false;

	void get_user_params(play_params* data);
	void process_cmdline_args(int argc, char** argv, const char** res);
	void run_user_loop(pa_data& data, tripple_buffer<play_params>* p_params);
};
