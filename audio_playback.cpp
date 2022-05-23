#include "audio_playback.h"

#define verify_pa_no_error_verbose(err)\
if( (err) != paNoError ) {\
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );\
    return err;\
}

#define SAMPLE_RATE (48000)
#define PA_SAMPLE_TYPE  paFloat32
#define NUM_CHANNELS 2

static librandom::simple random_gen;

int pa_player::playCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData) {

    paData* data = (paData*)userData;
    
    int frameIndex = data->frameIndex[data->fileID];
    // randomization happens here
    if (!frameIndex) {
        uint32_t rnd_id = random_gen.i(NUM_FILES - 1);
        librandom::cache cache(data->cacheId);
        rnd_id = cache.check(rnd_id, NUM_FILES);
        data->cacheId = cache.value();
        data->fileID = (int)rnd_id;
    }
    const int fileID = data->fileID;
    const AudioFile<float> &audioFile = data->audioFile[fileID];
    const AudioFile<float>::AudioBuffer *r_buf = &audioFile.samples;
    float* wptr = (float*)outputBuffer;
    int i;
    int framesLeft = audioFile.getNumSamplesPerChannel() - frameIndex;

    (void)inputBuffer; /* Prevent unused variable warnings. */
    (void)timeInfo;
    (void)statusFlags;

    const int num_channels = audioFile.getNumChannels();
    int file_offset = frameIndex;
    const int mono_file = 1 - (int)audioFile.isMono();
    if (framesLeft >= framesPerBuffer) {
        for (i = 0; i < framesPerBuffer; i++, file_offset++)
        {
            *wptr++ = (*r_buf)[0][file_offset];  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = (*r_buf)[mono_file][file_offset];  /* right */
        }
        data->frameIndex[fileID] += framesPerBuffer;
    }
    else {
        /* final buffer... */
        for (i = 0; i < framesLeft; i++, file_offset++)
        {
            *wptr++ = (*r_buf)[0][file_offset];  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = (*r_buf)[mono_file][file_offset];  /* right */
        }
        for (; i < framesPerBuffer; i++)
        {
            *wptr++ = 0;  /* left */
            if (NUM_CHANNELS == 2) *wptr++ = 0;  /* right */
        }
        data->frameIndex[fileID] = 0;
    }
    return paContinue;
}

int pa_player::init_pa(paData* data) {
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
        256,        /* frames per buffer, i.e. the number
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