#pragma once

#include <array>

#include "portaudio.h"
#include "AudioFile.h"
#include "librandom.h"

#define SAMPLE_RATE (48000)

#define _min(x, y) (x) < (y) ? (x) : (y)
#define _max(x, y) (x) > (y) ? (x) : (y)

struct play_data {
    const std::vector<AudioFile<float>>* audioFile;
    std::array<std::vector<float>, 2> processing_buffer;
    std::vector<int> frameIndex;
    int fileID;
    uint32_t cacheId;
    float pitch;
    float volume;
    int numStepFrames;
    bool waveshaper_enabled;

    void init(const std::vector<AudioFile<float>> *af) {
        audioFile = af;
        fileID = 0;
        cacheId = 0;
        pitch = 1.0f;
        volume = 1.0f;
        numStepFrames = 0;
        waveshaper_enabled = false;
        frameIndex.resize(audioFile->size(), 0);
    }
};

struct play_params {
    int numStepFrames;
    float pitch_deviation;
    float volume_lower_bound;
    float lpf_freq_range;
    float lpf_q_range;
    bool waveshaper_enabled;

    void init(int nsf, float pd, float vlb, float lfr, float lqr, bool we) {
        numStepFrames = nsf;
        pitch_deviation = pd;
        volume_lower_bound = vlb;
        lpf_freq_range = lfr;
        lpf_q_range = lqr;
        waveshaper_enabled = we;
    }
};

struct paData
{
    play_data p_data;
    play_params *p_params;

    paData(const std::vector<AudioFile<float>>* af) : p_params(nullptr) {
        p_data.init(af);
    }

    void resize_processing_buffer(int num_frames) {
        p_data.processing_buffer[0].resize(num_frames);
        p_data.processing_buffer[1].resize(num_frames);
    }
    void fill_buffer_with_silence() {
        const size_t size = p_data.processing_buffer[0].size();
        memset(p_data.processing_buffer[0].data(), 0, sizeof(float) * size);
        memset(p_data.processing_buffer[1].data(), 0, sizeof(float) * size);
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

    int init_pa(paData* );
    int deinit_pa();
};