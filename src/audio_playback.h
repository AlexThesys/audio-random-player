#pragma once

#include <array>

#include "AudioFile.h"
#include "audio_processing.h"
#include "librandom.h"
#include "portaudio.h"

struct play_data
{
    const audio_file_container *audio_file;
    buffer_container processing_buffer;
    std::vector<int> frame_index;
    int file_id;
    uint32_t cache_id;
    float pitch;
    float volume;
    int num_step_frames;
    bool use_lfo;
    bool waveshaper_enabled;
    // padding 2 bytes

    play_data()
        : audio_file(nullptr), file_id(0), cache_id(0), pitch(1.0f), volume(1.0f), num_step_frames(0), use_lfo(false),
          waveshaper_enabled(false)
    {
    }

    void init(const audio_file_container *af)
    {
        audio_file = af;
        file_id = 0;
        cache_id = 0;
        pitch = 1.0f;
        volume = 1.0f;
        num_step_frames = 0;
        waveshaper_enabled = false;
        use_lfo = false;
        frame_index.resize(audio_file->size(), 0);
    }
};

struct play_params
{
    int num_step_frames;
    float pitch_deviation;
    float volume_lower_bound;
    float lpf_freq_range;
    float lpf_q_range;
    float lfo_freq;
    float lfo_amount;
    bool use_lfo;
    bool waveshaper_enabled;
    // padding 2 bytes

    void init(int nsf, float pd, float vlb, float lfr, float lqr, float lf, float la, bool ul, bool we)
    {
        num_step_frames = nsf;
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

struct pa_data
{
    play_data p_data;
    play_params *volatile p_params;

    pa_data(const audio_file_container *af) : p_params(nullptr)
    {
        p_data.init(af);
    }

    // C5220
    pa_data(const pa_data&) = delete;
    pa_data& operator=(const pa_data&) = delete;
    pa_data(pa_data&&) = delete;
    pa_data& operator=(pa_data&&) = delete;

    void resize_processing_buffer(size_t num_frames)
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
    static int playCallback(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
        void* user_data);
    int init_pa(pa_data *data);
    int deinit_pa();
};