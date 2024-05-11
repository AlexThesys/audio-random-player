#pragma once

#include <thread>
#include <atomic>
#include <stdint.h>

#include <CL/cl.h>

#include "visualization.h"
#include "producer_consumer.h"


class compute_fft {
    static constexpr size_t num_streams = 2;

    struct compute_context {
        cl_context context;
        cl_command_queue command_queue[num_streams];
        cl_program program;
        cl_kernel kernel[1];
        cl_device_id device_id = 0;

        cl_mem input[num_streams];
        cl_mem output[3]; // tripple-buffered SSBOs

        cl_int init(const char* filename, const compute_fft* owner);
        cl_int deinit();
    } context;

    // waveform input from audio engine
    producer_consumer<waveform_data>* waveform_consumer = nullptr; // consumer of waveform_producer

    // output fft to graphics engine
    tripple_indices* ssbo_buffer_ids;  // indices into SSBO array
    struct {
        HGLRC hGLRC;
        HDC hDC;
        cl_uint ssbo[3]; // tripple-buffered SSBOs
    } gl_context;

    std::thread compute_thread;
    std::atomic<int> state_compute{1};

    const char* filename;

    cl_int queue_selector; // alternate between "frames"

    static void compute_mt(void* args);

public:
    compute_fft() : ssbo_buffer_ids(nullptr), filename(nullptr), queue_selector(0) 
    { 
        memset(gl_context.ssbo, 0, sizeof(gl_context.ssbo)); 
        gl_context.hGLRC = NULL;
        gl_context.hDC = NULL;
    }

    cl_int init(visualizer::fft_t &fft, producer_consumer<waveform_data> *wf_consumer);
    void deinit();
    void run();
};