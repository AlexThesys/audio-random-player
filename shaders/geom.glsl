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
	
	const vec4 position = vec4(gl_in[0].gl_Position.xy, 0.0f, 1.0f);
	const float offset = gl_in[0].gl_Position.z;
	vec4 base = position;
	base.y = offset;
	
	const float s = float(size);
	const float w = float(ubo.width);
	const float extend = (max(s , w) / (s*s)) * 0.25f;
	
	const vec3 colour = gs_in[0].colour;
	
    gl_Position = position;
	gl_Position.x -= extend;
	gs_out.colour = colour;
    EmitVertex();
    gl_Position = position;
	gl_Position.x += extend;
	gs_out.colour = colour;
    EmitVertex();
    gl_Position = base;
	gl_Position.x -= extend;
	gs_out.colour = colour;
    EmitVertex();
    gl_Position = base;
	gl_Position.x += extend;
	gs_out.colour = colour;
    EmitVertex();

    EndPrimitive();
}