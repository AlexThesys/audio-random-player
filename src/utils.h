#pragma once

#include <array>

#include "emmintrin.h"

using audio_file_container = std::array<AudioFile<float>, (1 << NUM_FILES_POW_2)>;
using buffer_container = std::array<std::vector<__m128>, NUM_CHANNELS>;
using output_buffer_container = std::vector<__m128>;
using viz_container = std::array<__m128i, FRAMES_PER_BUFFER * NUM_CHANNELS / FP_IN_VEC>; // big enough size is required to fit either s16 or floats

struct viz_data {
    viz_container container;
    bool fp_mode = false;
};

#define verify_pa_no_error_verbose(err)\
if( (err) != paNoError ) {\
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );\
    return err;\
}

#define _min(x, y) (x) < (y) ? (x) : (y)
#define _max(x, y) (x) > (y) ? (x) : (y)
#define clampr(val, min, max) _min(_max((val), (min)), (max))

template <typename T> 
int clamp_input(T val, T min, T max)
{
    val = _min(val, max);
    return _max(val, min);
}

inline bool fp_similar(float x, float y, float eps = 0.0001f) {
    return (fabs(x) - fabs(y)) < eps;
}