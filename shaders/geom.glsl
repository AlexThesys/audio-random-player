// geometry shader
#version 440 core

#define FRAMES_PER_BUFFER 0x100 // same as in constants.h

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (std140, binding = 1) uniform ubo_block {
	int width;
	int fp_mode;
} ubo;

in v_colour {
    vec3 colour;
}	gs_in[];

out g_colour {
    vec3 colour;
} gs_out;

void main()
{
	const int size = FRAMES_PER_BUFFER;
	
	const float s = float(size);
	const float w = float(ubo.width);
	const float extend = (max(s , w) / (s*s)) * 0.25f;
	
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x -= extend;
	gl_Position.y += extend;
	gs_out.colour = gs_in[0].colour;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x += extend;
	gl_Position.y += extend;
	gs_out.colour = gs_in[0].colour;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x -= extend;
	gl_Position.y -= extend;
	gs_out.colour = gs_in[0].colour;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
	gl_Position.x += extend;
	gl_Position.y -= extend;
	gs_out.colour = gs_in[0].colour;
    EmitVertex();

    EndPrimitive();
}