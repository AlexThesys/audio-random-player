#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

class r_shader
{
	std::unordered_map<const GLchar*, GLint> uniform_location;
	GLuint id;
public:
	r_shader() : id(0) {}
	bool load_shaders(const char* vs_file_path, const char* fs_file_path, const char* gs_file_path = nullptr);
	void use_program();
	GLuint get_program_id() const { return id; }
    bool validate_program();
	void delete_shader() { glDeleteProgram(id); }

	GLint get_uniform_location(const GLchar* name);

	void set_uniform(const GLchar* name, int x);
	void set_uniform_sampler(const GLchar* name, int slot);
	void set_uniform(const GLchar* name, float x);
	void set_uniform(const GLchar* name, bool cond);
	void set_uniform(const GLchar* name, const glm::vec2& v);
	void set_uniform(const GLchar* name, float x, float y);
	void set_uniform(const GLchar* name, const glm::vec3& v);
	void set_uniform(const GLchar* name, float x, float y, float z);
	void set_uniform(const GLchar* name, const glm::vec4& v);
	void set_uniform(const GLchar* name, float x, float y, float z, float w);
    void set_uniform(const GLchar * name, const glm::mat3& m);
    void set_uniform(const GLchar* name, const glm::mat4& m);

private:
	GLchar* file_to_string(const char* file_path);
    bool check_compile_errors(GLuint shader) const;
    bool check_link_errors() const;
    void get_attrib_info() const;
    void get_uniform_info() const;
};

inline GLint r_shader::get_uniform_location(const GLchar* name)
{
	auto it = uniform_location.find(name);
	if (it == uniform_location.end())
	{
		uniform_location[name] = glGetUniformLocation(id, name);
	}

	return uniform_location[name];
}

inline void r_shader::get_attrib_info() const
{
	GLint max_length, n_attribs;
	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &n_attribs);
	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);

	GLchar* name = (GLchar*)malloc(max_length);

	GLint written, size, location;
	GLenum type;
	printf("Index | Name\n");
	printf("----------------------------------------\n");
	for (int i = 0; i < n_attribs; ++i) {
		glGetActiveAttrib(id, i, max_length, &written,
			&size, &type, name);
		location = glGetAttribLocation(id, name);
		printf("%-5d | %s\n", location, name);
	}

	free(name);
}

inline void r_shader::get_uniform_info() const
{
	GLint max_length, num_uniforms;
	glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_uniforms);
	glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);

	GLchar* name = (GLchar*)malloc(max_length);

	GLint written, size, location;
	GLenum type;
	printf("Index | Name\n");
	printf("----------------------------------------\n");
	for (int i = 0; i < num_uniforms; ++i) {
		glGetActiveUniform(id, i, max_length, &written,
			&size, &type, name);
		location = glGetUniformLocation(id, name);
		printf("%-5d | %s\n", location, name);
	}

	free(name);

}

// set uniform variations

inline void r_shader::set_uniform(const GLchar* name, int x)
{
	GLint loc = get_uniform_location(name);
	glUniform1i(loc, x);
}

inline void r_shader::set_uniform(const GLchar* name, float x)
{
	GLint loc = get_uniform_location(name);
	glUniform1f(loc, x);
}

inline void r_shader::set_uniform(const GLchar* name, bool cond)
{
	GLint loc = get_uniform_location(name);
	glUniform1i(loc, cond);
}

inline void r_shader::set_uniform(const GLchar* name, const glm::vec2& v)
{
	GLint loc = get_uniform_location(name);
	glUniform2fv(loc, 1, &v[0]);
}

inline void r_shader::set_uniform(const GLchar* name, float x, float y)
{
	GLint loc = get_uniform_location(name);
	glUniform2f(loc, x, y);
}

inline void r_shader::set_uniform(const GLchar* name, const glm::vec3& v)
{
	GLint loc = get_uniform_location(name);
	glUniform3fv(loc, 1, &v[0]);
}

inline void r_shader::set_uniform(const GLchar* name, float x, float y, float z)
{
	GLint loc = get_uniform_location(name);
	glUniform3f(loc, x, y, z);
}

inline void r_shader::set_uniform(const GLchar* name, const glm::vec4& v)
{
	GLint loc = get_uniform_location(name);
	glUniform4fv(loc, 1, &v[0]);
}

inline void r_shader::set_uniform(const GLchar* name, float x, float y, float z, float w)
{
	GLint loc = get_uniform_location(name);
	glUniform4f(loc, x, y, z, w);
}

inline void r_shader::set_uniform(const GLchar* name, const glm::mat3& m)
{
    GLint loc = get_uniform_location(name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

inline void r_shader::set_uniform(const GLchar* name, const glm::mat4& m)
{
	GLint loc = get_uniform_location(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}


inline void r_shader::set_uniform_sampler(const GLchar* name, int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	GLint loc = get_uniform_location(name);
	glUniform1i(loc, slot);
}