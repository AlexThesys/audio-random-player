#include "Window.h"
#include "profiling.h"

int r_window::initialise(bool full_screen) {
	glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _APPLE_
	glfwWindowHInt(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // _APPLE_

	const GLchar* app_title = "audio_test";

	if (full_screen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* vMode = glfwGetVideoMode(monitor);
		if (vMode != nullptr)
			main_window = glfwCreateWindow(vMode->width, vMode->height, app_title, monitor, nullptr);
	}
	else
	{
		main_window = glfwCreateWindow(width, height, app_title, nullptr, nullptr);
	}
	if (main_window == nullptr)
	{
		puts("Failed to create GLFW widnow! Exiting...");
		glfwTerminate();
		return false;
	}

    glfwGetFramebufferSize(main_window, &buffer_width, &buffer_height);
    glfwSetFramebufferSizeCallback(main_window, on_frame_buffer_size);

	glfwMakeContextCurrent(main_window);

	create_callbacks();

	//glfwSetInputMode(main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(main_window, double(buffer_width) / 2.0, double(buffer_height) / 2.0);

	glewExperimental = GL_TRUE;
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK) {
		printf("glew initialization error: %d\n", glew_error);
		glfwDestroyWindow(main_window);
		glfwTerminate();
        return -1;
	}

	glViewport(0, 0, buffer_width, buffer_height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glfwSetWindowUserPointer(main_window, this);

	return 0;
}

void r_window::handle_keys(GLFWwindow * window, int key, int code, int action, int mode) {
	r_window *this_window = static_cast<r_window*>(glfwGetWindowUserPointer(window));

	//close the window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void r_window::create_callbacks() {
	glfwSetKeyCallback(main_window, handle_keys);
}

void r_window::on_frame_buffer_size(GLFWwindow *window, int width, int height)
{
	r_window * this_window = static_cast<r_window*>(glfwGetWindowUserPointer(window));
	this_window->buffer_width = width;
	this_window->buffer_height = height;
    glViewport(0, 0, width, height);
}

