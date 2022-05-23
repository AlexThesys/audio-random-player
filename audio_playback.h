#pragma once

#include <array>

#include "portaudio.h"
#include "AudioFile.h"
#include "librandom.h"

#define NUM_FILES 4
#define SAMPLE_RATE (48000)

struct paData
{
    AudioFile<float> *audioFile;
    int frameIndex[NUM_FILES];
    int fileID;
    uint32_t cacheId;
    int numStepFrames;
    float pitch;
    float volume;
    float pitch_deviation;
    float volume_lower_bound;
};

class pa_player {
private:
    PaStream *_stream;
public:
    pa_player() : _stream(nullptr) {}
    static int playCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);

    int init_pa(paData*);
    int deinit_pa();
};