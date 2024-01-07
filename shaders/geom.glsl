// geometry shader
#version 440 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform int width;
uniform int size;

void main()
{
	const float s = float(size);
	const float w = float(width);
	const float extend = (max(s , w) / (s*s)) * 0.25f;
	
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x -= extend;
	gl_Position.y += extend;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x += extend;
	gl_Position.y += extend;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x -= extend;
	gl_Position.y -= extend;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x += extend;
	gl_Position.y -= extend;
    EmitVertex();

    EndPrimitive();
}