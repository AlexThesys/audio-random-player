#pragma once

#include "dsp.h"

size_t resample(const AudioFile<float>::AudioBuffer &source, buffer_container &dest, size_t file_offset,
                size_t in_samples, size_t out_samples, size_t frames_per_buffer, size_t num_ch, bool fadeout);

void apply_volume(buffer_container &buffer, size_t num_channels, float volume, bool use_lfo,
                  dsp::modulation::lfo &lfo_gen);