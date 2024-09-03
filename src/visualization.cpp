#include <chrono>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

#include "AudioFile.h"
#include "visualization.h"
#include "constants.h"
#include "compute.h"
#include "profiling.h"

// binding points - same as in the shaders
#define SSBO_BINDING_POINT_0 0
#define SSBO_BINDING_POINT_1 1
#define SSBO_BINDING_POINT_2 2
#define SSBO_BINDING_POINT_3 3
#define UBO_BINDING_POINT 4

namespace {
    struct ubo_block {
        int32_t width;
        int32_t fp_mode;
    };
}

visualizer::visualizer()
{
    compute_cl = nullptr;

    waveform.VAO = 0;
    waveform.SSBO = 0;
    waveform.waveform_smoothing_level = VIZ_BUFFER_SMOOTHING_LEVEL_DEF;

    fft.VAO = 0;
    memset(fft.SSBO, 0, sizeof(fft.SSBO));
    fft.hGLRC = 0;
    fft.hDC = 0;

    UBO = 0;
    frame = 0;
}

bool visualizer::init_gl()
{
    // window initialization
    window.initialise();

    // create shaders
    if (!waveform.shader.load_shaders("shaders/wf_vert.glsl", "shaders/wf_frag.glsl", "shaders/wf_geom.glsl")) {
        goto error;
    } 
    if (!waveform.shader.validate_program()) {
        goto error;
    }

    if (!fft.shader.load_shaders("shaders/fft_vert.glsl", "shaders/fft_frag.glsl", "shaders/fft_geom.glsl")) {
        goto error;
    }
    if (!fft.shader.validate_program()) {
        goto error;
    }

    // UBO -- shared between the shader programs
    glGenBuffers(1, &UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ubo_block), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_BINDING_POINT, UBO);

//--------- waveform -------------------------------------------------------
    // create buffers
    glGenVertexArrays(1, &waveform.VAO);
    glGenBuffers(1, &waveform.SSBO);

    glBindVertexArray(waveform.VAO);

    // SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, waveform.SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(int32_t) * waveform.waveform_smoothing_level, nullptr, GL_DYNAMIC_DRAW); // allocating a big enough buffer to fit either s16 or floats
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING_POINT_0, waveform.SSBO);

    glBindVertexArray(0);

    //--------- fft -------------------------------------------------------
    glGenVertexArrays(1, &fft.VAO);
    glGenBuffers(3, fft.SSBO);

    glBindVertexArray(fft.VAO);

    // SSBO
    int8_t zero_buf[VIZ_BUFFER_SIZE * sizeof(glm::vec2)];
    memset(zero_buf, 0, sizeof(zero_buf)); // zeroize input buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fft.SSBO[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(glm::vec2), zero_buf, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fft.SSBO[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(glm::vec2), zero_buf, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fft.SSBO[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(glm::vec2), zero_buf, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING_POINT_1, fft.SSBO[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING_POINT_2, fft.SSBO[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING_POINT_3, fft.SSBO[2]);

    glBindVertexArray(0);

    //fft.hGLRC = wglGetCurrentContext();
    //fft.hDC = wglGetCurrentDC();
    fft.hGLRC = glfwGetWGLContext(window.get_window());
    const HWND hwnd = glfwGetWin32Window(window.get_window());
    fft.hDC = GetDC(hwnd);

    glFinish();

    return true;

error:
    fft.sem_cl.signal(); // release cl thread
    return false;
}

void visualizer::run_gl()
{
    const std::chrono::microseconds frame_time(TARGET_FPS_mcs);
    auto prev_tm = std::chrono::high_resolution_clock::now();
    while(!window.get_should_close() && state_render.load())
    {
        // limit fps to 60
        const auto curr_tm = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curr_tm - prev_tm);
        if (duration.count() < TARGET_FPS_mcs) {
            std::this_thread::sleep_for(frame_time - duration);
        }
        prev_tm = curr_tm;

        PROFILE_START("visualizer::run_gl");

        glClear(GL_COLOR_BUFFER_BIT);

        waveform_data* wf_data_front_buf_ptr = waveform.waveform_buffer->consume();
        const bool fp_mode = wf_data_front_buf_ptr->fp_mode;
        const size_t data_size = !fp_mode ? sizeof(int16_t) : sizeof(float);

        // UBO -- shared between the shader programs
        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        ubo_block ubo_data = { window.get_buffer_width() , int32_t(fp_mode) };
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo_block), &ubo_data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // draw waveform
        {          
            waveform.shader.use_program();

            glBindVertexArray(waveform.VAO);
            // SSBO
            const int32_t ssbo_frame = int32_t(frame % waveform.waveform_smoothing_level);
            const size_t ssbo_size = VIZ_BUFFER_SIZE * data_size;
            const size_t ssbo_offset = ssbo_size * ssbo_frame;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, waveform.SSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, ssbo_offset, ssbo_size, wf_data_front_buf_ptr->container.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            // other uniforms 
            waveform.shader.set_uniform("smoothing_level", waveform.waveform_smoothing_level); // used to calculate the offset within the SSBO

            glDrawArrays(GL_POINTS, 0, VIZ_BUFFER_SIZE);
        }

        // draw fft
        {
            fft.shader.use_program();

            glBindVertexArray(fft.VAO);
            // other uniforms 
            const int ssbo_id = (int)fft.ssbo_buffer_ids.consume();
            fft.shader.set_uniform("buffer_selector", ssbo_id); // used to select the SSBO

            glDrawArrays(GL_POINTS, 0, VIZ_BUFFER_SIZE);
        }

        glBindVertexArray(0);
        glUseProgram(0);
        // swap buffers
        window.swap_buffers();
        // process input
        glfwPollEvents();

        frame++;

        PROFILE_FRAME("Render");
        PROFILE_STOP("visualizer::run_gl");
    }
    if (compute_cl) {
        compute_cl->state_compute.store(0);
        fft.sem_cl.signal();
    }
}

void visualizer::deinit_gl()
{
    glDeleteVertexArrays(1, &waveform.VAO);
    glDeleteBuffers(1, &waveform.SSBO);
    waveform.shader.delete_shader();
    glDeleteVertexArrays(1, &fft.VAO);
    waveform.shader.delete_shader();
    window.deinitialize();
    if (compute_cl) {
        fft.sem_cl.wait();
    }
    glDeleteBuffers(3, fft.SSBO);
}

void visualizer::init(triple_buffer<waveform_data>* wf_buf, int32_t smoothing_level, compute_fft* fft_comp)
{
    waveform.waveform_buffer = wf_buf;
    waveform.waveform_smoothing_level = smoothing_level;
    compute_cl = fft_comp;

    render_thread = std::thread(render, this);
}

void visualizer::render(void* args) 
{
    PROFILE_SET_THREAD_NAME("Graphics/Render");

    visualizer *self = (visualizer*)args;

    if (self->init_gl()) {
        self->compute_cl->init(self->fft);
        self->fft.sem_cl.signal(); // signal cl thread that ssbo is ready
        self->run_gl();
        self->deinit_gl();
    } else {
        self->waveform.shader.delete_shader();
    }
}