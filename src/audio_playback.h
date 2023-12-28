#pragma once

#include <array>

#include "AudioFile.h"
#include "portaudio.h"
#include "audio_processing.h"
#include "cache.h"
#include "circular_buffer.h"

struct play_data
{
    const audio_file_container *audio_file;
    buffer_container processing_buffer;
    int frame_index;
    int frame_counter;
    int file_id;
    uint32_t cache_id;
    float pitch;
    float volume;
    int num_step_frames;
    // 4 bytes padding

    play_data()
        : audio_file(nullptr), frame_index(0), frame_counter(0), file_id(0), cache_id(0), pitch(1.0f), volume(1.0f),
          num_step_frames(0)
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
        frame_index = 0;
        frame_counter = 0;
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
    play_params *front_buf;

    pa_data(const audio_file_container *af) : front_buf(nullptr)
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
        assert((num_frames & (FP_IN_VEC - 1)) == 0x0);
        num_frames /= FP_IN_VEC;
        p_data.processing_buffer[0].resize(num_frames);
        p_data.processing_buffer[1].resize(num_frames);
    }
    void fill_buffer_with_silence()
    {
        const size_t size = p_data.processing_buffer[0].size();
        memset(p_data.processing_buffer[0].data(), 0, sizeof(__m128) * size);
        memset(p_data.processing_buffer[1].data(), 0, sizeof(__m128) * size);
    }
    void randomize_data();
    void process_audio(float* out_buffer, size_t frames_per_buffer);
};

class audio_renderer; // FWD

class pa_player
{
  private:
    PaStream *_stream;

  public:
    pa_player() : _stream(nullptr)
    {
    }
    int init_pa(audio_renderer* renderer);
    int deinit_pa();
};

class audio_renderer 
{
    pa_data* data = nullptr;
    std::array<output_buffer_container, (1 << NUM_BUFFERS_POW_2)> buffers;
    size_t buffer_idx = 0;
    circular_buffer<output_buffer_container*, NUM_BUFFERS_POW_2> buffer_queue; // thread-safe
    std::thread render_thread;
    std::atomic<int> state_render{1};
public:
    void init(pa_data* data);
    void deinit();

    static int fill_output_buffer(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                            void* user_data);
private:
    static void render(void* renderer);
    void process_data();
};