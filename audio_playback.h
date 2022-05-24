#pragma once

#include <array>

#include "portaudio.h"
#include "AudioFile.h"
#include "librandom.h"

#define NUM_FILES 4
#define SAMPLE_RATE (48000)

#define _min(x, y) (x) < (y) ? (x) : (y)
#define _max(x, y) (x) > (y) ? (x) : (y)

struct paData
{
    AudioFile<float> *audioFile;
    std::array<std::vector<float>, 2> processing_buffer;
    int frameIndex[NUM_FILES];
    int fileID;
    uint32_t cacheId;
    int numStepFrames;
    float pitch;
    float volume;
    float pitch_deviation;
    float volume_lower_bound;
    bool waveshaper_enabled;

    paData(AudioFile<float>* af, int nsf, float pd, float vlb, bool we) : audioFile(af), fileID(0), cacheId(0), numStepFrames(nsf), 
            pitch(1.0f), volume(1.0f), pitch_deviation(pd), volume_lower_bound(vlb), waveshaper_enabled(we) {

        memset(frameIndex, 0, sizeof(frameIndex));
    }
    void resize_processing_buffer(int num_frames) {
        processing_buffer[0].resize(num_frames);
        processing_buffer[1].resize(num_frames);
    }
    void fill_buffer_with_silence() {
        const size_t size = processing_buffer[0].size();
        memset(processing_buffer[0].data(), 0, sizeof(float) * size);
        memset(processing_buffer[1].data(), 0, sizeof(float) * size);
    }
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