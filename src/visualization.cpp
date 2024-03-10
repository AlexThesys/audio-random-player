#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

#include "AudioFile.h"
#include "visualization.h"
#include "constants.h"
#include "profiling.h"

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
    shader.use_program();
    shader.set_uniform("size", FRAMES_PER_BUFFER); // maybe just set as a constant in the shader?
    glUseProgram(0);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &SSBO);

    glBindVertexArray(VAO);

    //position
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VIZ_BUFFER_SIZE * sizeof(int32_t), nullptr, GL_DYNAMIC_DRAW); // allocating a big enough buffer to fit either s16 or floats
    const GLuint bind_point = 0u;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_point, SSBO);
    GLuint block_inndex = glGetUniformBlockIndex(shader.get_program_id(), "storage_block");
    glUniformBlockBinding(shader.get_program_id(), block_inndex, bind_point);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindVertexArray(0);

    return true;
}

void visualizer::run_gl()
{
    while(!window.get_should_close() && state_render.load())
    {
        viz_data* viz_data_front_buf_ptr = viz_data_buffer->consume();
        const bool fp_mode = viz_data_front_buf_ptr->fp_mode;
        const size_t data_size = !fp_mode ? sizeof(int16_t) : sizeof(float);
        glClear(GL_COLOR_BUFFER_BIT);   
        // draw
        shader.use_program();
        shader.set_uniform("width", window.get_buffer_width());
        shader.set_uniform("fp_mode", fp_mode);

        glBindVertexArray(VAO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, VIZ_BUFFER_SIZE * data_size, viz_data_front_buf_ptr->container.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glDrawArrays(GL_POINTS, 0, VIZ_BUFFER_SIZE);
        glBindVertexArray(0);

        // swap buffers
        glUseProgram(0);
        window.swap_buffers();
        // process input
        glfwPollEvents();

        PROFILE_FRAME("Render");
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