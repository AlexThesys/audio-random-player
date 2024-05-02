#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "audio_playback.h"
#include "constants.h"
#include "utils.h"
#include "profiling.h"

#define PA_SAMPLE_TYPE paFloat32

static librandom::randu random_gen;
static dsp::filter lp_filter;
static const dsp::modulation::wavetable w_table;
static dsp::modulation::lfo lfo_gen{&w_table};

static inline float semitones_to_pitch_scale(float semitones_dev)
{
    const float semitones = random_gen.fp(-semitones_dev, semitones_dev);
    return powf(2.0f, semitones / 12.0f);
}

void pa_data::randomize_data(play_params* params_front_buffer)
{
    uparams = params_front_buffer;
    p_data.pitch = semitones_to_pitch_scale(params_front_buffer->pitch_deviation);
    p_data.volume = random_gen.fp(params_front_buffer->volume_lower_bound, MAX_VOLUME);
    const float lpf_freq = random_gen.fp(MAX_LPF_FREQ - params_front_buffer->lpf_freq_range, MAX_LPF_FREQ);
    const float lpf_q = random_gen.fp(DEFAULT_LPF_Q, DEFAULT_LPF_Q + params_front_buffer->lpf_q_range);
    lp_filter.setup(lpf_freq, lpf_q);
    p_data.num_step_frames = params_front_buffer->num_step_frames;
    if (params_front_buffer->use_lfo)
        lfo_gen.set_rate(params_front_buffer->lfo_freq, params_front_buffer->lfo_amount);
    p_data.frame_index = 0;
}

void pa_data::process_audio(float *out_buffer, size_t frames_per_buffer)
{
    assert((frames_per_buffer & 0x3) == 0x0);
    const int frame_index = p_data.frame_index;
    const AudioFile<float> &audio_file = *p_data.audio_file;
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

        apply_volume(processing_buffer, num_file_channels, p_data.volume, uparams->use_lfo, lfo_gen);

        if (uparams->waveshaper_enabled)
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

bool audio_renderer::init(const char* folder_path, size_t* max_lenght_samples)
{
    data = new pa_data();
    streamer = new audio_streamer();
    params_buffer = new tripple_buffer<play_params>();
    viz_data_buffer = new tripple_buffer<viz_data>();
    if (!streamer->init(folder_path, max_lenght_samples)) {
        delete streamer;
        delete data;
        delete params_buffer;
        delete viz_data_buffer;
        return false;
    }
    static_assert((FRAMES_PER_BUFFER & (FP_IN_VEC - 1)) == 0x0, "Frame buffer size should be divisible by 4.");
    data->resize_processing_buffer(FRAMES_PER_BUFFER);

    const __m128 zero = _mm_setzero_ps();
    for (auto& buf : buffers) {
        buf.resize(FRAMES_PER_BUFFER * NUM_CHANNELS / FP_IN_VEC, zero);
    }

    zero_out_viz_data();

    return true;
}

void audio_renderer::start_rendering() 
{
    render_thread = std::thread(render, this);
}

void audio_renderer::deinit() 
{
    state_render.store(0);
    if (render_thread.joinable()) {
        render_thread.join();
    }
    streamer->deinit();
    delete streamer;
    delete data;
    delete params_buffer;
    delete viz_data_buffer;
}

int audio_renderer::fill_output_buffer(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                                        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                                        void* user_data)
{
    PROFILE_SET_TREAD_NAME("Audio/Submit");


    PROFILE_FRAME_START("Audio");
    PROFILE_START("audio_renderer::fill_output_buffer");
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
    if (renderer->buffer_queue.try_read(output)) {
        memcpy(output_buffer, output->data(), buffer_size_bytes); // TODO: check the disassembly

        renderer->submit_viz_data(output); // would be better to do this in audio render thread, not in audio callback

        renderer->buffer_queue.advance();
    } else {
        memset(output_buffer, 0, buffer_size_bytes);
        // zero out viz_data - would be better to do this in audio render thread, not in audio callback
        auto* viz_data_buffer = renderer->get_viz_data_buffer();
        viz_data* viz_data_back_buffer_ptr = viz_data_buffer->get_back_buffer();
        memset(viz_data_back_buffer_ptr->container.data(), 0, buffer_size_bytes);
        viz_data_buffer->publish();
    }
    PROFILE_STOP("audio_renderer::fill_output_buffer");
    PROFILE_FRAME_STOP("Audio");

    return paContinue;
}

void audio_renderer::render(void* arg)
{
    PROFILE_SET_TREAD_NAME("Audio/Render");

    audio_renderer &renderer = *(audio_renderer*)arg;
    static const auto mcs = std::chrono::microseconds(1000);
    while (renderer.state_render.load()) {
        if (!renderer.buffer_queue.is_full()) {
            renderer.process_data();
        } else {
            std::this_thread::sleep_for(mcs);
        }
    }
}

void audio_renderer::process_data()
{
    PROFILE_START("audio_renderer::process_data");
    if (!data->p_data.frame_counter) {
        data->p_data.audio_file = streamer->request();
        assert(data->p_data.audio_file);
        data->randomize_data(params_buffer->consume());
    }
    output_buffer_container *output = &buffers[buffer_idx];
    data->process_audio(reinterpret_cast<float*>(output->data()), static_cast<size_t>(FRAMES_PER_BUFFER));
    bool res = buffer_queue.try_push(output); res;
    assert(res);
    buffer_idx = ++buffer_idx & (buffers.size() - 1);
    PROFILE_STOP("audio_renderer::process_data");
}

void audio_renderer::submit_viz_data(const output_buffer_container* output)
{
    PROFILE_START("audio_renderer::submit_viz_data");

    viz_data* viz_data_back_buffer_ptr = viz_data_buffer->get_back_buffer();
    viz_container& container = viz_data_back_buffer_ptr->container;
    const bool fp_mode = data->uparams->fp_visualization;
    if (!fp_mode) {
        constexpr int container_size = FRAMES_PER_BUFFER * NUM_CHANNELS / S16_IN_VEC;
        // default s16 visualization
        constexpr float s16_min = -32768.0f;
        constexpr float s16_max = 32767.0f;
        constexpr float range = s16_max - s16_min;
        const __m128 min_input = _mm_set1_ps(-1.0f);
        const __m128 min_output = _mm_set1_ps(s16_min);
        const __m128 range_output = _mm_set1_ps(range);
        const __m128 denominator_rec = _mm_set1_ps(0.5f);

        // convert float to s16
        for (int i = 0, j = 0; i < container_size; i++, j += 2) {
            const __m128 input_0 = (*output)[j];
            const __m128 input_1 = (*output)[j + 1];

            __m128 numerator = _mm_sub_ps(input_0, min_input);
            __m128 division_result = _mm_mul_ps(numerator, denominator_rec);
            __m128 multiplication_result = _mm_mul_ps(division_result, range_output);
            const __m128 result_0 = _mm_add_ps(multiplication_result, min_output);

            numerator = _mm_sub_ps(input_1, min_input);
            division_result = _mm_mul_ps(numerator, denominator_rec);
            multiplication_result = _mm_mul_ps(division_result, range_output);
            const __m128 result_1 = _mm_add_ps(multiplication_result, min_output);

            // Convert the result to short integers
            const __m128i result_0_int = _mm_cvtps_epi32(result_0);
            const __m128i result_1_int = _mm_cvtps_epi32(result_1);
            // low 4 words - from result_0_int, high 4 words - from result_1_int
            container[i] = _mm_packs_epi32(result_0_int, result_1_int);
        }
    } else {
        constexpr size_t buffer_size_bytes = FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(float);
        memcpy(container.data(), output->data(), buffer_size_bytes);
    }
    viz_data_back_buffer_ptr->fp_mode = fp_mode;
    viz_data_buffer->publish();

    PROFILE_STOP("audio_renderer::submit_viz_data");
}

bool audio_streamer::init(const char* folder_path, size_t* max_lenght_samples)
{
    if (!load_file_names(folder_path, max_lenght_samples)) {
        return false;
    }

    // push the file batch of files right away
    for (const auto &_ : audio_files) {
        load_file();
    }

    streamer_thread = std::thread(stream, this);

    return true;
}

void audio_streamer::deinit()
{
    state_streaming.store(0);
    sem.signal();
    if (streamer_thread.joinable()) {
        streamer_thread.join();
    }
}

AudioFile<float>* audio_streamer::request()
{
    AudioFile<float>* file = nullptr;
    // we first advance the read index and only then start reading, so that the data wouldn't been overriden during the process
    file_queue.advance();
    file_queue.read_tail(file);
    sem.signal(); // signal to load the next file
    return file;
}

void audio_streamer::stream(void* arg)
{
    PROFILE_SET_TREAD_NAME("Audio/Streamer");

    audio_streamer* streamer = (audio_streamer*)arg;

    while (streamer->state_streaming.load()) {
        if (streamer->file_queue.is_empty()) {
            streamer->load_file();
        } else {
            streamer->sem.wait();
        }
    }
}

size_t audio_streamer::get_rnd_file_id() 
{
    const int num_files = static_cast<int>(file_names.size());
    uint32_t rnd_id = static_cast<uint32_t>(random_gen.i(num_files - 1));
    cache cache(cache_id);
    rnd_id = cache.check(rnd_id, static_cast<uint8_t>(num_files));
    cache_id = cache.value();
    return static_cast<const size_t>(rnd_id);
}

void audio_streamer::load_file() 
{
    PROFILE_START("audio_streamer::load_file");
    AudioFile<float>& audio_file = audio_files[buffer_idx];
    const size_t file_id = get_rnd_file_id();
    const bool res = audio_file.load(file_names[file_id]); res;
    assert(res);
    file_queue.try_push(&audio_file);
    buffer_idx = ++buffer_idx & (audio_files.size() - 1);
    PROFILE_STOP("audio_streamer::load_file");

}

bool audio_streamer::load_file_names(const char* folder_path, size_t* max_lenght_samples)
{
    char path[MAX_PATH];
    memset(path, 0, sizeof(path));
    strcpy_s(path, folder_path);
    strcat_s(path, "\\*.wav");

    size_t max_size = 0;

    WIN32_FIND_DATA fd;
    HANDLE h_find = ::FindFirstFile(path, &fd);
    if (h_find != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                memset(path, 0, sizeof(path));
                strcpy_s(path, folder_path);
                strcat_s(path, "\\");
                strcat_s(path, fd.cFileName);

                AudioFile<float> audio_file;
                audio_file.load(path);
                // check size
                const size_t num_samples = audio_file.getNumSamplesPerChannel();
                const size_t size = num_samples * audio_file.getNumChannels() * sizeof(float); // for fp32 wav format
                if (size < MAX_DATA_SIZE) {
                    file_names.push_back(std::string(path));
                    max_size = _max(num_samples, max_size);
                }
            }
        } while (::FindNextFile(h_find, &fd));
        ::FindClose(h_find);
        *max_lenght_samples = max_size;
        return !!file_names.size();
    }
    return false;
}