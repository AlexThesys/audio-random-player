#pragma once

#include "audio_playback.h"

struct user_params {
	size_t max_lenght_samples;
	char folder_path[MAX_PATH];
	// cmd args
	int32_t waveform_smoothing_level;
	bool disable_fadeout;

public:
	user_params() : max_lenght_samples(0), waveform_smoothing_level(VIZ_BUFFER_SMOOTHING_LEVEL_DEF), disable_fadeout(false) {}
	bool get_folder_path();
	void get_user_params(play_params* data);
	void process_cmdline_args(int argc, char** argv);
	void run_user_loop(pa_data& data, triple_buffer<play_params>* p_params);
};
