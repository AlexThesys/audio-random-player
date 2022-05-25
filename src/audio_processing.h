#pragma once

#include "dsp.h"

int resample(const AudioFile<float>::AudioBuffer &source, buffer_container &dest, int file_offset, int in_samples,
             int out_samples, int frames_per_buffer, int num_ch, bool fadeout);

void apply_volume(buffer_container &buffer, float volume, bool use_lfo, dsp::wavetable &lfo_gen);