#pragma once

#include <array>

#include "emmintrin.h"

using audio_file_container = std::array<AudioFile<float>, (1 << NUM_FILES_POW_2)>;
using buffer_container = std::array<std::vector<__m128>, NUM_CHANNELS>;
using output_buffer_container = std::vector<__m128>;
using viz_container = std::array<__m128i, FRAMES_PER_BUFFER * NUM_CHANNELS / FP_IN_VEC>;

#define verify_pa_no_error_verbose(err)\
if( (err) != paNoError ) {\
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );\
    return err;\
}

#define _min(x, y) (x) < (y) ? (x) : (y)
#define _max(x, y) (x) > (y) ? (x) : (y)

template <typename T> 
int clamp_input(T val, T min, T max)
{
    val = _min(val, max);
    return _max(val, min);
}