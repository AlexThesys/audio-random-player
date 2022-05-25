#include "audio_playback.h"
#include "constants.h"
#include "utils.h"

#define PA_SAMPLE_TYPE paFloat32

static librandom::simple random_gen;
static dsp::filter lp_filter;
static dsp::wavetable lfo_gen;

inline float semitones_to_pitch_scale(float semitones_dev)
{
    const float semitones = random_gen.f(-semitones_dev, semitones_dev);
    return powf(2.0f, semitones / 12.0f);
}

static void randomize_data(pa_data *data)
{
    const int num_files = (int)data->p_data.audio_file->size();
    uint32_t rnd_id = (uint32_t)random_gen.i(num_files - 1);
    cache cache(data->p_data.cache_id);
    rnd_id = cache.check(rnd_id, (uint8_t)num_files);
    data->p_data.cache_id = cache.value();
    data->p_data.file_id = (int)rnd_id;
    data->p_data.pitch = semitones_to_pitch_scale(data->p_params->pitch_deviation);
    data->p_data.volume = random_gen.f(data->p_params->volume_lower_bound, MAX_VOLUME);
    const float lpf_freq = random_gen.f(MAX_LPF_FREQ - data->p_params->lpf_freq_range, MAX_LPF_FREQ);
    const float lpf_q = random_gen.f(DEFAULT_LPF_Q, DEFAULT_LPF_Q + data->p_params->lpf_q_range);
    lp_filter.setup(lpf_freq, lpf_q);
    data->p_data.num_step_frames = data->p_params->num_step_frames;
    data->p_data.use_lfo = data->p_params->use_lfo;
    if (data->p_data.use_lfo)
        lfo_gen.set_rate(data->p_params->lfo_freq, data->p_params->lfo_amount);
    data->p_data.waveshaper_enabled = data->p_params->waveshaper_enabled;
    data->p_data.frame_index[(size_t)data->p_data.file_id] = 0;
}

inline void process_audio(float *out_buffer, pa_data *data, size_t frames_per_buffer)
{
    const float volume = data->p_data.volume;
    const size_t file_id = (size_t)data->p_data.file_id;
    const int frame_index = data->p_data.frame_index[file_id];
    const int frame_counter = data->p_data.frame_counter[file_id];
    const bool waveshaper_enabled = data->p_data.waveshaper_enabled;
    const AudioFile<float> &audio_file = (*data->p_data.audio_file)[file_id];
    const size_t stereo_file = (size_t)audio_file.isStereo();
    const int total_samples = _min(audio_file.getNumSamplesPerChannel(), (int)data->p_data.num_step_frames);
    size_t i;
    const int audio_frames_left = total_samples - frame_index;
    if (audio_frames_left > 0) {
        const int out_samples = _min(audio_frames_left, (int)frames_per_buffer);
        const int in_samples = (int)(float(out_samples) * data->p_data.pitch);
        buffer_container &processing_buffer = data->p_data.processing_buffer;

        const int frames_read =
            (int)resample(audio_file.samples, processing_buffer, (size_t)frame_index, (size_t)in_samples,
                          (size_t)out_samples, frames_per_buffer, (size_t)audio_file.getNumChannels(),
                          (total_samples < audio_file.getNumSamplesPerChannel()));

        apply_volume(processing_buffer, volume, data->p_data.use_lfo, lfo_gen);

        if (waveshaper_enabled)
            dsp::waveshaper::process(processing_buffer, dsp::waveshaper::default_params);

        lp_filter.process(processing_buffer);

        for (i = 0; i < frames_per_buffer; i++) {
            *out_buffer++ = processing_buffer[0][i]; /* left */
            if (NUM_CHANNELS == 2)
                *out_buffer++ = processing_buffer[stereo_file][i]; /* right */
        }
        data->p_data.frame_index[file_id] += frames_read;
        data->p_data.frame_counter[file_id] += (int)frames_per_buffer;
    } else {
        for (i = 0; i < frames_per_buffer; i++) {
            *out_buffer++ = 0; /* left */
            if (NUM_CHANNELS == 2)
                *out_buffer++ = 0; /* right */
        }
        if (frame_counter < data->p_data.num_step_frames) {
            data->p_data.frame_counter[file_id] += (int)frames_per_buffer;
        } else {
            data->p_data.frame_counter[file_id] = 0;
        }
    }
}

int pa_player::playCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags,
                            void *user_data)
{
    //assert(FRAMES_PER_BUFFER == framesPerBuffer);
    if (FRAMES_PER_BUFFER != frames_per_buffer) {
        return paAbort;
    }

    (void)input_buffer; /* Prevent unused variable warnings. */
    (void)time_info;
    (void)status_flags;

    pa_data *data = (pa_data *)user_data;

    if (!data->p_data.frame_counter[(size_t)data->p_data.file_id])
        randomize_data(data);

    process_audio((float *)output_buffer, data, (size_t)frames_per_buffer);

    return paContinue;
}

int pa_player::init_pa(pa_data *data)
{
    data->resize_processing_buffer(FRAMES_PER_BUFFER);

    PaError err = Pa_Initialize();
    verify_pa_no_error_verbose(err);

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        return -1;
    }
    outputParameters.channelCount = 2; /* stereo output */
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
                        paClipOff, pa_player::playCallback, /* this is your callback function */
                        (void *)data);                      /*This is a pointer that will be passed to
                                                                 your callback*/
    verify_pa_no_error_verbose(err);

    err = Pa_StartStream(_stream);

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