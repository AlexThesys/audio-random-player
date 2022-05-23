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

static librandom::simple random_gen;

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
    if (!data->frameIndex[data->fileID]) {
        uint32_t rnd_id = random_gen.i(NUM_FILES - 1);
        librandom::cache cache(data->cacheId);
        rnd_id = cache.check(rnd_id, NUM_FILES);
        data->cacheId = cache.value();
        data->fileID = (int)rnd_id;
        data->pitch = semitones_to_pitch_scale(data->pitch_deviation);
        data->volume = random_gen.f(data->volume_lower_bound, 1.0f);
    }
    const float volume = data->volume;
    const int fileID = data->fileID;
    const int frameIndex = data->frameIndex[fileID];
    const AudioFile<float> &audioFile = data->audioFile[fileID];
    float* wptr = (float*)outputBuffer;
    int i;
    int audioFramesLeft = audioFile.getNumSamplesPerChannel() - frameIndex;

    (void)inputBuffer; /* Prevent unused variable warnings. */
    (void)timeInfo;
    (void)statusFlags;

    const int num_channels = audioFile.getNumChannels();
    const int stereo_file = (int)audioFile.isStereo();
    if (frameIndex < audioFile.getNumSamplesPerChannel()) {
        const int out_samples = _min(audioFramesLeft, (int)framesPerBuffer);
        const int in_samples = roundf(float(out_samples) * data->pitch);
        std::array<std::vector<float>, 2>& processing_buffer = data->processing_buffer;
        resample(audioFile.samples, processing_buffer, frameIndex, in_samples, out_samples, (int)framesPerBuffer, volume, audioFile.getNumChannels());

        for (i = 0; i < framesPerBuffer; i++) {
            *wptr++ = processing_buffer[0][i];  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = processing_buffer[stereo_file][i];  /* right */
        }
        data->frameIndex[fileID] += in_samples;
    } else {
        if (frameIndex < data->numStepFrames) {
            for (i = 0; i < framesPerBuffer; i++) {
                *wptr++ = 0;  /* left */
                if (NUM_CHANNELS == 2) *wptr++ = 0;  /* right */
            }
            data->frameIndex[fileID] += framesPerBuffer;
        } else {
            data->frameIndex[fileID] = 0;
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
        data); /*This is a pointer that will be passed to
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

////////////////////////////

/*
PaError     Pa_IsStreamStopped (PaStream *stream)
PaError     Pa_IsStreamActive (PaStream *stream)
const PaStreamInfo *    Pa_GetStreamInfo (PaStream *stream)
PaTime  Pa_GetStreamTime (PaStream *stream)
PaError Pa_GetSampleSize (PaSampleFormat format)
void    Pa_Sleep (long msec)

    //int sampleRate = audioFile.getSampleRate();
    //int bitDepth = audioFile.getBitDepth();

    //int numSamples = audioFile.getNumSamplesPerChannel();
    //double lengthInSeconds = audioFile.getLengthInSeconds();

    //int numChannels = audioFile.getNumChannels();
    //bool isMono = audioFile.isMono();
    //bool isStereo = audioFile.isStereo();

    // or, just use this quick shortcut to print a summary to the console
    //audioFile.printSummary();
*/