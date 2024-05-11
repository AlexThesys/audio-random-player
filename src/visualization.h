#pragma once

#include <atomic>
#include <thread>

#include <GL/glew.h>
#include "Window.h"
#include "Shader.h"
#include "tripple_buffer.h"
#include "semaphore.h"
#include "AudioFile.h"
#include "utils.h"

class visualizer
{
public:
    struct waveform_t {
        tripple_buffer<waveform_data>* waveform_buffer = nullptr;
        r_shader shader;
        GLuint VAO;
        GLuint SSBO;
        int32_t waveform_smoothing_level;
    };

    struct fft_t {
        tripple_indices ssbo_buffer_ids; // indices into SSBO array
        r_shader shader;
        GLuint VAO;
        GLuint SSBO[3]; // tripple-buffered SSBOs
        semaphore sem_cl; // used to signal that fft ssbos are ready
        HGLRC hGLRC;
        HDC hDC;
    };
    
    GLuint UBO; // shared between shader programms

private:
    waveform_t waveform;
    fft_t fft;

    r_window window;
    uint64_t frame;

    std::thread render_thread;
    std::atomic<int> state_render{1};

public:
    visualizer();

    void init(tripple_buffer<waveform_data>* wf_buf, int32_t smoothing_level);
    void deinit() 
    { 
        state_render.store(0); 
        if (render_thread.joinable()) {
            render_thread.join();
        }
    }

    static void render(void *args);

    fft_t& get_fft_data() { return fft; }

private:
    bool init_gl();
    void run_gl();
    void deinit_gl();
};