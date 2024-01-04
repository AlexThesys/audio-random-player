#pragma once

#include <cstdio>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

class r_window {
private:
	GLFWwindow *main_window;
	GLint width, height;
	GLint buffer_width, buffer_height;
	void create_callbacks();
	static void handle_keys(GLFWwindow *window, int key, int code, int action, int mode);
    static void on_frame_buffer_size(GLFWwindow * window, int width, int height);
public:
	
	r_window(GLint w = 768, GLint h = 768) : width(w), height(h), buffer_width(0), buffer_height(0), main_window(nullptr) {}
	int initialise(bool full_screen=false);
	void deinitialize();
	bool get_should_close(){ return glfwWindowShouldClose(main_window); }
	void swap_buffers(){ glfwSwapBuffers(main_window); }
};

inline void r_window::deinitialize() 
{
	glfwDestroyWindow(main_window);
	glfwTerminate();
}
