// vertex shader
#version 440 core

layout (location=0) in float y_position;

uniform int width;

#define min_s16 -32768.0f
#define max_s16 32767.0f

void main()
{
    const float x_pos = (float(gl_VertexID) / float(width)) * 2.0f - 1.0f;
	
	const int is_right_channel = gl_VertexID & 0x01;
	const float offset = 0.25f - 0.5f * float(is_right_channel);
	const float scale = 0.5f;
	const float y_pos = y_position * scale + offset;
	
    gl_Position = vec4(x_pos, y_pos, 0.0f, 1.0f);
}