#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

#include "visualization.h"
#include "constants.h"

GLfloat vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

bool visualizer::init_gl()
{
    // window initialization
    window.initialise();

    // create shaders
    if (!shader.load_shaders("../shaders/vert.glsl", "../shaders/frag.glsl"))// , "../shaders/geom.glsl"))
        return false;
    
    if (!shader.validate_program())
        return false;

    shader.use_program();
    shader.set_uniform("size", (int)VIZ_BUFFER_SIZE);
    glUseProgram(0);

//----------------------------------------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    //position
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, GL_FALSE, (void*)nullptr);
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
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, VIZ_BUFFER_SIZE);
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

void visualizer::init() 
{
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