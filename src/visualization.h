#pragma once

#include <atomic>
#include <thread>

#include <GL/glew.h>
#include "Window.h"
#include "Shader.h"
#include "triple_buffer.h"
#include "semaphore.h"
#include "AudioFile.h"
#include "utils.h"

class compute_fft; // FWD

class visualizer
{
public:
    struct waveform_t {
        triple_buffer<waveform_data>* waveform_buffer = nullptr;
        r_shader shader;
        GLuint VAO;
        GLuint SSBO;
        int32_t waveform_smoothing_level;
    };

    struct fft_t {
        triple_indices ssbo_buffer_ids; // indices into SSBO array
        r_shader shader;
        GLuint VAO;
        GLuint SSBO[3]; // tripple-buffered SSBOs
        semaphore sem_cl; // used to signal that fft ssbos are ready
        HGLRC hGLRC;
        HDC hDC;
    };
    
    GLuint UBO; // shared between the shader programs

private:
    compute_fft* compute_cl;

    waveform_t waveform;
    fft_t fft;

    r_window window;
    uint64_t frame;

    std::thread render_thread;
    std::atomic<int> state_render{1};

public:
    visualizer();

    void init(triple_buffer<waveform_data>* wf_buf, int32_t smoothing_level, compute_fft* fft_comp);
    void deinit() 
    { 
        state_render.store(0); 
        if (render_thread.joinable()) {
            render_thread.join();
        }
    }

    semaphore& get_cl_sem() { return fft.sem_cl; }

    static void render(void *args);

private:
    bool init_gl();
    void run_gl();
    void deinit_gl();
};