#pragma once

#include "xmmintrin.h"

using audio_file_container = std::vector<AudioFile<float>>;
using buffer_container = std::array<std::vector<__m128>, 2>;

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