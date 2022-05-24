#include "audio_playback.h"
#include "audio_processing.h"

#define verify_pa_no_error_verbose(err)\
if( (err) != paNoError ) {\
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );\
    return err;\
}

#define PA_SAMPLE_TYPE  paFloat32
#define NUM_CHANNELS 2
#define FRAMES_PER_BUFFER 256
#define MAX_VOLUME 1.0f
#define MAX_LPF_FREQ 20000.0f
#define DEFAULT_LPF_Q 0.707f

static librandom::simple random_gen;
static dsp::filter lp_filter;

inline float semitones_to_pitch_scale(float semitones_dev) {
    const float semitones = random_gen.f(-semitones_dev, semitones_dev);
    return powf(2.0f, semitones / 12.0f);
}

int pa_player::playCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData) {
    assert(FRAMES_PER_BUFFER == framesPerBuffer);
    paData* data = (paData*)userData;
    
    // randomization happens here
    if (!data->p_data.frameIndex[data->p_data.fileID]) {
        const int num_files = data->p_data.audioFile->size();
        uint32_t rnd_id = random_gen.i(num_files - 1);
        librandom::cache cache(data->p_data.cacheId);
        rnd_id = cache.check(rnd_id, num_files);
        data->p_data.cacheId = cache.value();
        data->p_data.fileID = (int)rnd_id;
        data->p_data.pitch = semitones_to_pitch_scale(data->p_params->pitch_deviation);
        data->p_data.volume = random_gen.f(data->p_params->volume_lower_bound, MAX_VOLUME);
        const float lpf_freq = random_gen.f(MAX_LPF_FREQ - data->p_params->lpf_freq_range, MAX_LPF_FREQ);
        const float lpf_q = random_gen.f(DEFAULT_LPF_Q, DEFAULT_LPF_Q + data->p_params->lpf_q_range);
        lp_filter.setup(lpf_freq, lpf_q);
        data->p_data.numStepFrames = data->p_params->numStepFrames;
        data->p_data.waveshaper_enabled = data->p_params->waveshaper_enabled;
    }
    const float volume = data->p_data.volume;
    const int fileID = data->p_data.fileID;
    const int frameIndex = data->p_data.frameIndex[fileID];
    const AudioFile<float>& audioFile = (*data->p_data.audioFile)[fileID];
    float* wptr = (float*)outputBuffer;
    int i;

    (void)inputBuffer; /* Prevent unused variable warnings. */
    (void)timeInfo;
    (void)statusFlags;

    const int num_channels = audioFile.getNumChannels();
    const int stereo_file = (int)audioFile.isStereo();
    const int total_samples = _min(audioFile.getNumSamplesPerChannel(), data->p_data.numStepFrames);
    if (frameIndex < total_samples) {
        const int audioFramesLeft = total_samples - frameIndex;
        const int out_samples = _min(audioFramesLeft, (int)framesPerBuffer);
        const int in_samples = (int)(float(out_samples) * data->p_data.pitch);
        std::array<std::vector<float>, 2>& processing_buffer = data->p_data.processing_buffer;
        
        const int frames_read = resample(audioFile.samples, processing_buffer, frameIndex, in_samples, 
            out_samples, (int)framesPerBuffer, volume, 
            audioFile.getNumChannels());

        if (data->p_data.waveshaper_enabled)
            dsp::waveshaper::process(processing_buffer, dsp::waveshaper::default_params);

        lp_filter.process(processing_buffer);

        for (i = 0; i < framesPerBuffer; i++) {
            *wptr++ = processing_buffer[0][i];  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = processing_buffer[stereo_file][i];  /* right */
        }
        data->p_data.frameIndex[fileID] += frames_read;
    } else {
        for (i = 0; i < framesPerBuffer; i++) {
            *wptr++ = 0;  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = 0;  /* right */
        }
        if (frameIndex < data->p_data.numStepFrames) {
            data->p_data.frameIndex[fileID] += framesPerBuffer;
        } else {
            data->p_data.frameIndex[fileID] = 0;
        }
    }
    return paContinue;
}

int pa_player::init_pa(paData* data) {
    data->resize_processing_buffer(FRAMES_PER_BUFFER);

    PaError err = Pa_Initialize();
    verify_pa_no_error_verbose(err);
    
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        return -1;

    }
    outputParameters.channelCount = 2;                     /* stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    /* Open an audio I/O stream. */
    err = Pa_OpenStream(&_stream, // Pa_OpenDefaultStream
        NULL,          /* no input channels */
        &outputParameters,          /* stereo output */
        SAMPLE_RATE,
        FRAMES_PER_BUFFER, /* frames per buffer, i.e. the number
                           of sample frames that PortAudio will
                           request from the callback. Many apps
                           may want to use
                           paFramesPerBufferUnspecified, which
                           tells PortAudio to pick the best,
                           possibly changing, buffer size.*/
        paClipOff,
        pa_player::playCallback, /* this is your callback function */
        (void*)data); /*This is a pointer that will be passed to
                           your callback*/
    verify_pa_no_error_verbose(err);
 
    err = Pa_StartStream(_stream);
}

int pa_player::deinit_pa() {
    PaError err = Pa_StopStream(_stream);    // Pa_AbortStream()
    verify_pa_no_error_verbose(err);

    err = Pa_CloseStream(_stream);
    verify_pa_no_error_verbose(err);

    err = Pa_Terminate();
    verify_pa_no_error_verbose(err);
}