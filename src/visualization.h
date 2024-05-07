#pragma once

#include <atomic>
#include <thread>

#include <GL/glew.h>
#include "Window.h"
#include "Shader.h"
#include "tripple_buffer.h"
#include "utils.h"

class visualizer
{
    r_window window;
    r_shader shader;
    GLuint VAO;
    GLuint SSBO;
    GLuint UBO;

    tripple_buffer<viz_data>* viz_data_buffer = nullptr;
    uint64_t frame;
    int32_t viz_smoothing_level;

    std::atomic<int> state_render{1};
    std::thread render_thread;

public:
    visualizer() : VAO(0), SSBO(0), UBO(0), frame(0), viz_smoothing_level(VIZ_BUFFER_SMOOTHING_LEVEL_DEF) {}

    void init(tripple_buffer<viz_data>* viz_buf, int32_t smoothing_level);
    void deinit() 
    { 
        state_render.store(0); 
        if (render_thread.joinable()) 
            render_thread.join(); 
    }

    static void render(void *args);

private:
    bool init_gl();
    void run_gl();
    void deinit_gl();
};