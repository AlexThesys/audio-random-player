#include "Shader.h"

#include <cstdio>
#include <cstdlib>


bool r_shader::load_shaders(const char* vs_file_path, const char* fs_file_path, const char* gs_file_path)
{
    // caller is responsible for the deallocation
    GLchar* vs_code = nullptr; 
    GLchar* fs_code = nullptr;
    GLchar* gs_code = nullptr; 
    GLuint v_shader = 0;
    GLuint f_shader = 0;
    GLuint g_shader = 0;

	vs_code = file_to_string(vs_file_path);
	fs_code = file_to_string(fs_file_path);

	v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &vs_code, nullptr);
	glCompileShader(v_shader);
    if (!check_compile_errors(v_shader))
        goto exit;

	f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &fs_code, nullptr);
	glCompileShader(f_shader);
    if (!check_compile_errors(f_shader))
        goto exit;

	if (gs_file_path != nullptr)
	{
        gs_code = file_to_string(gs_file_path);

        g_shader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(g_shader, 1, &gs_code, nullptr);
        glCompileShader(g_shader);
        if (!check_compile_errors(g_shader))
            goto exit;
	}


	id = glCreateProgram();
	glAttachShader(id, v_shader);
	glAttachShader(id, f_shader);
	if(gs_file_path != nullptr) glAttachShader(id, g_shader);
	glLinkProgram(id);
    if (!check_link_errors())
        goto exit;

    get_attrib_info();
    get_uniform_info();

exit:
	glDeleteShader(v_shader);
	glDeleteShader(f_shader);
    if (gs_file_path) {
        glDeleteShader(g_shader);
    }
	uniform_location.clear();

    free(vs_code);
    free(fs_code);
    free(gs_code);

	return true;
}

void r_shader::use_program()
{
	if (id > 0)
		glUseProgram(id);
}

bool r_shader::validate_program() {
    GLint result = 0;
    GLchar eLog[1024] = { 0 };
    glValidateProgram(id);
    glGetProgramiv(id, GL_VALIDATE_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(id, sizeof(eLog), nullptr, eLog);
        printf("Error validating program: %s\n", eLog);
    }
    return !!result;
}

static long get_file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

GLchar* r_shader::file_to_string(const char* file_path)
{
    FILE* file = nullptr;
    
    if ((fopen_s(&file, file_path, "rb") != 0) || !file) {
        perror("Error opening file");
        return nullptr;
    }
    const long file_size = get_file_size(file);
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        return nullptr;
    }
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    if (bytes_read != file_size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return nullptr;
    }
    fclose(file);

    return buffer;
}

bool r_shader::check_compile_errors(GLuint shader) const
{
	int status = 0;
	GLint length = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		GLchar* error_log = (GLchar*)calloc(length, sizeof(char));
        glGetShaderInfoLog(id, length, &length, error_log);
		printf("Error compiling shader: %s\n", error_log);
		free(error_log);
    }
    return !!status;
}

bool r_shader::check_link_errors() const
{
    int status = 0;
    GLint length = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
		GLchar* error_log = (GLchar*)calloc(length, sizeof(char));
        glGetProgramInfoLog(id, length, &length, error_log);
		printf("Error linking program: %s\n", error_log);
		free(error_log);
    }
    return !!status;
}

