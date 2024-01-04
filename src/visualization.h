#pragma once

#include <atomic>
#include <thread>

#include <GL/glew.h>
#include "Window.h"
#include "Shader.h"

class visualizer
{
    r_window window;
    r_shader shader;
    GLuint VAO;
    GLuint VBO;

    std::atomic<int> state_render{1};
    std::thread render_thread;

public:
    void init();
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