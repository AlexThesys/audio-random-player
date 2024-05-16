// vertex shader
#version 440 core

#define FRAMES_PER_BUFFER 0x100 // same as in constants.h
#define NUM_CHANNELS 2 // same as in constants.h
#define VIZ_BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS) // same as in constants.h

#define WF_SCALE 0.5f

layout (std430, binding = 1) buffer storage_block_0 {
	vec2 data[];
} sbo_0;

layout (std430, binding = 2) buffer storage_block_1 {
	vec2 data[];
} sbo_1;

layout (std430, binding = 3) buffer storage_block_2 {
	vec2 data[];
} sbo_2;

uniform int buffer_selector;

out v_colour {
    vec3 colour;
}	vs_out;

void main()
{
	const int size_frames = FRAMES_PER_BUFFER;
	
	const int is_right_channel = gl_VertexID & 0x01;
	
	vec2 fft;
	switch (buffer_selector) {
		case 0 :
			fft = sbo_0.data[gl_VertexID];
			break;
		case 1 :
			fft = sbo_1.data[gl_VertexID];
			break;
		case 2 :
			fft = sbo_2.data[gl_VertexID];
			break;
		default : // shouldn't happend
			fft = vec2(0.0f, 0.0f);
			break;
	};
	
	const float amp = fft.x;
	const float phase = fft.y;
	
	const float x_pos = float(gl_VertexID & (~1)) / float(size_frames) - 1.0f; // == float(gl_VertexID & (~1)) * 0.5f / float(size_frames)) * 2.0f - 1.0f;;
	
	
	const float y_scale = WF_SCALE;
	const float y_pos = amp * y_scale * (1.0f - 2.0f * float(is_right_channel)); // flip right channel's direction
	
	vs_out.colour = vec3(phase, (1.0f - phase), 0.0f); // TODO: come up with something better
	
    gl_Position = vec4(x_pos, y_pos, 0.0f, 1.0f);
}