#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

#include "AudioFile.h"
#include "visualization.h"
#include "constants.h"

bool visualizer::init_gl()
{
    // window initialization
    window.initialise();

    // create shaders
    if (!shader.load_shaders("../shaders/vert.glsl", "../shaders/frag.glsl"))// , "../shaders/geom.glsl"))
        return false;
    
    if (!shader.validate_program())
        return false;

//----------------------------------------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    //position
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VIZ_BUFFER_SIZE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 1, GL_FLOAT, 0, GL_FALSE, (void*)nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void visualizer::run_gl()
{
    while(!window.get_should_close() && state_render.load())
    {
        glClear(GL_COLOR_BUFFER_BIT);   
        // draw
        shader.use_program();
        shader.set_uniform("width", FRAMES_PER_BUFFER); // maybe just set as a constant in the shader?
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        viz_container* viz_data_front_buf_ptr = viz_data_buffer->consume();
        glBufferSubData(GL_ARRAY_BUFFER, 0, VIZ_BUFFER_SIZE * sizeof(float), viz_data_front_buf_ptr->data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_POINTS, 0, VIZ_BUFFER_SIZE);
        glBindVertexArray(0);

        // swap buffers
        glUseProgram(0);
        window.swap_buffers();
        // process input
        glfwPollEvents();
    }
}

void visualizer::deinit_gl()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shader.delete_shader();
    window.deinitialize();
}

void visualizer::init(tripple_buffer<viz_container>* viz_buf)
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