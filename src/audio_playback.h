#pragma once

#include <array>

#include "AudioFile.h"
#include "audio_processing.h"
#include "librandom.h"
#include "portaudio.h"

struct play_data
{
    const audio_file_container *audioFile;
    buffer_container processing_buffer;
    std::vector<int> frameIndex;
    int fileID;
    uint32_t cacheId;
    float pitch;
    float volume;
    int numStepFrames;
    bool use_lfo;
    bool waveshaper_enabled;

    void init(const audio_file_container *af)
    {
        audioFile = af;
        fileID = 0;
        cacheId = 0;
        pitch = 1.0f;
        volume = 1.0f;
        numStepFrames = 0;
        waveshaper_enabled = false;
        use_lfo = false;
        frameIndex.resize(audioFile->size(), 0);
    }
};

struct play_params
{
    int numStepFrames;
    float pitch_deviation;
    float volume_lower_bound;
    float lpf_freq_range;
    float lpf_q_range;
    float lfo_freq;
    float lfo_amount;
    bool use_lfo;
    bool waveshaper_enabled;

    void init(int nsf, float pd, float vlb, float lfr, float lqr, float lf, float la, bool ul, bool we)
    {
        numStepFrames = nsf;
        pitch_deviation = pd;
        volume_lower_bound = vlb;
        lpf_freq_range = lfr;
        lpf_q_range = lqr;
        lfo_freq = lf;
        lfo_amount = la;
        use_lfo = ul;
        waveshaper_enabled = we;
    }
};

struct paData
{
    play_data p_data;
    play_params *volatile p_params;

    paData(const audio_file_container *af) : p_params(nullptr)
    {
        p_data.init(af);
    }

    void resize_processing_buffer(int num_frames)
    {
        p_data.processing_buffer[0].resize(num_frames);
        p_data.processing_buffer[1].resize(num_frames);
    }
    void fill_buffer_with_silence()
    {
        const size_t size = p_data.processing_buffer[0].size();
        memset(p_data.processing_buffer[0].data(), 0, sizeof(float) * size);
        memset(p_data.processing_buffer[1].data(), 0, sizeof(float) * size);
    }
};

class pa_player
{
  private:
    PaStream *_stream;

  public:
    pa_player() : _stream(nullptr)
    {
    }
    static int playCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                            void *userData);

    int init_pa(paData *);
    int deinit_pa();
};