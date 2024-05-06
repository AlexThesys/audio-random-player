#include <chrono>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

#include "AudioFile.h"
#include "visualization.h"
#include "constants.h"
#include "profiling.h"

// binding points - same as in the shaders
#define SSBO_BINDING_POINT 0
#define UBO_BINDING_POINT 1

static struct ubo_block {
    int width;
    int fp_mode;
};

bool visualizer::init_gl()
{
    // window initialization
    window.initialise();

    // create shaders
    if (!shader.load_shaders("shaders/vert.glsl", "shaders/frag.glsl", "shaders/geom.glsl"))
        return false;
    
    if (!shader.validate_program())
        return false;

//----------------------------------------------------------------
    // create buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &SSBO);
    glGenBuffers(1, &UBO);

    glBindVertexArray(VAO);

    // SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(int32_t), nullptr, GL_DYNAMIC_DRAW); // allocating a big enough buffer to fit either s16 or floats
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING_POINT, SSBO);
    // UBO
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ubo_block), nullptr, GL_STATIC_DRAW); // allocating a big enough buffer to fit either s16 or floats
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_BINDING_POINT, UBO);

    glBindVertexArray(0);

    return true;
}

void visualizer::run_gl()
{
    PROFILE_SET_THREAD_NAME("Graphics/Render");

    const std::chrono::microseconds frame_time(TARGET_FPS_mcs);
    auto prev_tm = std::chrono::high_resolution_clock::now();
    while(!window.get_should_close() && state_render.load())
    {
        const auto curr_tm = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curr_tm - prev_tm);
        if (duration.count() < TARGET_FPS_mcs) {
            std::this_thread::sleep_for(frame_time - duration);
        }

        PROFILE_START("visualizer::run_gl");

        prev_tm = curr_tm;

        viz_data* viz_data_front_buf_ptr = viz_data_buffer->consume();
        const bool fp_mode = viz_data_front_buf_ptr->fp_mode;
        const size_t data_size = !fp_mode ? sizeof(int16_t) : sizeof(float);
        glClear(GL_COLOR_BUFFER_BIT);   
        // draw
        shader.use_program();

        glBindVertexArray(VAO);
        // SSBO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, VIZ_BUFFER_SIZE * data_size, viz_data_front_buf_ptr->container.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        // UBO
        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        ubo_block ubo_data = { window.get_buffer_width() , int(fp_mode) };
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo_block), &ubo_data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glDrawArrays(GL_POINTS, 0, VIZ_BUFFER_SIZE);

        glBindVertexArray(0);

        glUseProgram(0);
        // swap buffers
        window.swap_buffers();
        // process input
        glfwPollEvents();

        PROFILE_FRAME("Render");
        PROFILE_STOP("visualizer::run_gl");
    }
}

void visualizer::deinit_gl()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &SSBO);
    shader.delete_shader();
    window.deinitialize();
}

void visualizer::init(tripple_buffer<viz_data>* viz_buf)
{
    viz_data_buffer = viz_buf;
    render_thread = std::thread(render, this);
}

void visualizer::render(void* args) 
{
    visualizer *viz = (visualizer*)args;

    if (viz->init_gl()) {
        viz->run_gl();
        viz->deinit_gl();
    } else {
        viz->shader.delete_shader();
    }
}