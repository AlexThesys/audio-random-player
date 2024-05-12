// geometry shader
#version 440 core

#define FRAMES_PER_BUFFER 0x100 // same as in constants.h
#define WF_SCALE 0.5f

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (std140, binding = 4) uniform ubo_block {
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
	vec4 base = position;
	base.y = gl_in[0].gl_Position.z; // y_offset
	
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
	gs_out.colour = colour; // vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = base;
	gl_Position.x += extend;
	gs_out.colour = colour; // vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();

    EndPrimitive();
}