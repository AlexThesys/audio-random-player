#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "audio_playback.h"
#include "constants.h"
#include "utils.h"

#define PA_SAMPLE_TYPE paFloat32

static librandom::simple random_gen;
static dsp::filter lp_filter;
static const dsp::modulation::wavetable w_table;
static dsp::modulation::lfo lfo_gen{&w_table};

extern volatile play_params *middle_buf;
extern volatile LONG new_data;

static inline float semitones_to_pitch_scale(float semitones_dev)
{
    const float semitones = random_gen.f(-semitones_dev, semitones_dev);
    return powf(2.0f, semitones / 12.0f);
}

void pa_data::randomize_data()
{
    if (InterlockedCompareExchange(&new_data, 0, 1))
        front_buf = (play_params*)InterlockedExchange64((volatile LONG64*)&middle_buf, reinterpret_cast<LONG64>(front_buf));
    const int num_files = static_cast<int>(p_data.audio_file->size());
    uint32_t rnd_id = static_cast<uint32_t>(random_gen.i(num_files - 1));
    cache cache(p_data.cache_id);
    rnd_id = cache.check(rnd_id, static_cast<uint8_t>(num_files));
    p_data.cache_id = cache.value();
    p_data.file_id = static_cast<int>(rnd_id);
    p_data.pitch = semitones_to_pitch_scale(front_buf->pitch_deviation);
    p_data.volume = random_gen.f(front_buf->volume_lower_bound, MAX_VOLUME);
    const float lpf_freq = random_gen.f(MAX_LPF_FREQ - front_buf->lpf_freq_range, MAX_LPF_FREQ);
    const float lpf_q = random_gen.f(DEFAULT_LPF_Q, DEFAULT_LPF_Q + front_buf->lpf_q_range);
    lp_filter.setup(lpf_freq, lpf_q);
    p_data.num_step_frames = front_buf->num_step_frames;
    if (front_buf->use_lfo)
        lfo_gen.set_rate(front_buf->lfo_freq, front_buf->lfo_amount);
    p_data.frame_index = 0;
}

void pa_data::process_audio(float *out_buffer, size_t frames_per_buffer)
{
    assert((frames_per_buffer & 0x3) == 0x0);
    const int frame_index = p_data.frame_index;
    const AudioFile<float> &audio_file = (*p_data.audio_file)[static_cast<size_t>(p_data.file_id)];
    const int total_samples = _min(audio_file.getNumSamplesPerChannel(), (int)p_data.num_step_frames);
    const int audio_frames_left = total_samples - frame_index;
    const size_t num_file_channels = static_cast<size_t>(audio_file.getNumChannels());
    if (audio_frames_left > 0) {
        const int out_samples = _min(audio_frames_left, static_cast<int>(frames_per_buffer));
        const int in_samples = static_cast<int>(static_cast<float>(out_samples) * p_data.pitch);
        buffer_container &processing_buffer = p_data.processing_buffer;

        const int frames_read = static_cast<int>(resample(
            audio_file.samples, processing_buffer, static_cast<size_t>(frame_index), static_cast<size_t>(in_samples),
            static_cast<size_t>(out_samples), frames_per_buffer, num_file_channels,
            (total_samples < audio_file.getNumSamplesPerChannel())));

        apply_volume(processing_buffer, num_file_channels, p_data.volume, front_buf->use_lfo, lfo_gen);

        if (front_buf->waveshaper_enabled)
            dsp::waveshaper::process(processing_buffer, num_file_channels, dsp::waveshaper::default_params);

        lp_filter.process(processing_buffer, num_file_channels);

        const size_t stereo_file = static_cast<size_t>(audio_file.isStereo());
        for (size_t i = 0; i < frames_per_buffer; i+=4, out_buffer+=FP_IN_VEC*2) {
            // PA output buffer uses interleaved frame format
            out_buffer[0] = ((float*)processing_buffer[0].data())[i + 0];
            out_buffer[1] = ((float*)processing_buffer[stereo_file].data())[i + 0];
            out_buffer[2] = ((float*)processing_buffer[0].data())[i + 1];
            out_buffer[3] = ((float*)processing_buffer[stereo_file].data())[i + 1];
            out_buffer[4] = ((float*)processing_buffer[0].data())[i + 2];
            out_buffer[5] = ((float*)processing_buffer[stereo_file].data())[i + 2];
            out_buffer[6] = ((float*)processing_buffer[0].data())[i + 3];
            out_buffer[7] = ((float*)processing_buffer[stereo_file].data())[i + 3];
        }

        p_data.frame_index += frames_read;
    } else {
        memset(out_buffer, 0, frames_per_buffer * sizeof(float) * NUM_CHANNELS);    /* left & right */
    }
    const int frame_counter = p_data.frame_counter + static_cast<int>(frames_per_buffer);
    p_data.frame_counter = (frame_counter < p_data.num_step_frames) ? frame_counter : 0;
}

int pa_player::init_pa(audio_renderer* renderer)
{
    PaError err = Pa_Initialize();
    verify_pa_no_error_verbose(err);

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        return -1;
    }
    static_assert(NUM_CHANNELS == 2, "Only stereo configuration supported for now.");
    outputParameters.channelCount = NUM_CHANNELS; /* stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    /* Open an audio I/O stream. */
    err = Pa_OpenStream(&_stream,                           // Pa_OpenDefaultStream
                        NULL,                               /* no input channels */
                        &outputParameters,                  /* stereo output */
                        SAMPLE_RATE, FRAMES_PER_BUFFER,     /* frames per buffer, i.e. the number
                                                            of sample frames that PortAudio will
                                                            request from the callback. Many apps
                                                            may want to use
                                                            paFramesPerBufferUnspecified, which
                                                            tells PortAudio to pick the best,
                                                            possibly changing, buffer size.*/
                        paClipOff, audio_renderer::fill_output_buffer, /* this is your callback function */
                        (void *)renderer);                  /*This is a pointer that will be passed to
                                                                 your callback*/
    verify_pa_no_error_verbose(err);
    if (err != paNoError) {
        err = Pa_Terminate();
        verify_pa_no_error_verbose(err);
    }

    const PaStreamInfo *info = Pa_GetStreamInfo(_stream);
    if (info && (static_cast<int>(info->sampleRate) == SAMPLE_RATE)) {
        err = Pa_StartStream(_stream);
        verify_pa_no_error_verbose(err);
    } else {
        puts("Default sample rate unsupported - check your audio device!");
        err = -1;
    }

    if (err != paNoError) {
        err = Pa_CloseStream(_stream);
        verify_pa_no_error_verbose(err);
        err = Pa_Terminate();
        verify_pa_no_error_verbose(err);
    }

    return paNoError;
}

int pa_player::deinit_pa()
{
    PaError err = Pa_StopStream(_stream); // Pa_AbortStream()
    verify_pa_no_error_verbose(err);

    err = Pa_CloseStream(_stream);
    verify_pa_no_error_verbose(err);

    err = Pa_Terminate();
    verify_pa_no_error_verbose(err);

    return paNoError;
}

void audio_renderer::init(pa_data* pdata) 
{
    data = pdata;
    static_assert((FRAMES_PER_BUFFER & (FP_IN_VEC - 1)) == 0x0, "Frame buffer size should be divisible by 4.");
    data->resize_processing_buffer(FRAMES_PER_BUFFER);

    const __m128 zero = _mm_setzero_ps();
    for (auto &buf : buffers) {
        buf.resize(FRAMES_PER_BUFFER * NUM_CHANNELS, zero);
    }
    render_thread = std::thread(render, this);
}

void audio_renderer::deinit() 
{
    state_render.store(0);
    render_thread.join();
}

int audio_renderer::fill_output_buffer(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                                        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                                        void* user_data)
{
    //assert(FRAMES_PER_BUFFER == framesPerBuffer);
    if (FRAMES_PER_BUFFER != frames_per_buffer) {
        return paAbort;
    }

    (void)input_buffer; /* Prevent unused variable warnings. */
    (void)time_info;
    (void)status_flags;

    audio_renderer* renderer = reinterpret_cast<audio_renderer*>(user_data);
    
    output_buffer_container* output = nullptr;
    constexpr size_t buffer_size_bytes = FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(float);
    if (renderer->buffer_queue.try_pop_task_queue(output)) {
        memcpy(output_buffer, output->data(), buffer_size_bytes); // TODO: check the disassembly
    } else {
        memset(output_buffer, 0, buffer_size_bytes);
    }
    return paContinue;
}

void audio_renderer::render(void* arg)
{
    audio_renderer &renderer = *(audio_renderer*)arg;
    static const auto mcs = std::chrono::microseconds(1000);
    while (renderer.state_render.load()) {
        if (!renderer.buffer_queue.is_full_task_queue()) {
            renderer.process_data();
        } else {
            std::this_thread::sleep_for(mcs);
        }
    }
}

void audio_renderer::process_data()
{
    if (!data->p_data.frame_counter) {
        data->randomize_data();
    }
    output_buffer_container *output = &buffers[buffer_idx];
    data->process_audio(reinterpret_cast<float*>(output->data()), static_cast<size_t>(FRAMES_PER_BUFFER));
    if (!buffer_queue.try_push_task_queue(output)) {
        assert(false);
    }
    buffer_idx = ++buffer_idx & (buffers.size() - 1);
}