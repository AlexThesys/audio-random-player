// vertex shader
#version 440 core

#define FRAMES_PER_BUFFER 0x100 // same as in constants.h
#define NUM_CHANNELS 2 // same as in constants.h
#define VIZ_BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS) // same as in constants.h

#define WF_SCALE 0.5f

#define s16s_in_s32 2
#define s16_min 32768.0f
#define s16_max 32767.0f
#define conversion_factor (2.0f / (s16_max + s16_min))
#define s16_mask 0x0000FFFF
#define s16_bits 0x10
#define s16_sign_bit 0x8000

layout (std430, binding = 0) buffer storage_block {
	int y_pos_data[]; // can accept both s16 and floats
} sbo;

layout (std140, binding = 3) uniform ubo_block {
	int width;
	int fp_mode;
	int num_buffers;
} ubo;

out v_colour {
    vec3 colour;
}	vs_out;

float s16_to_float(int val_packed, int selector)
{
	// extract either low or high word
	int val = (val_packed >> (selector * s16_bits)) & s16_mask;
	// restore the sign
	//val = int(uint(val) << s16_mask) >> s16_mask;
	int sign_ext = int((s16_sign_bit & val) == s16_sign_bit);
	sign_ext = -sign_ext;
	sign_ext &= ~s16_mask;
	val |= sign_ext;
	
	return ((float(val) + s16_min) * conversion_factor) - 1.0f;
}

void main()
{
	const int size_frames = FRAMES_PER_BUFFER;
	const int buffer_size = VIZ_BUFFER_SIZE / (s16s_in_s32 - ubo.fp_mode);
	const int num_buffers = ubo.num_buffers;
	int buffer_offset = 0;
	
	const int is_right_channel = gl_VertexID & 0x01;
	
	float y_position = 0.0f;
	if (ubo.fp_mode == 0) {
		for (int i = 0; i < num_buffers; i++) {
			y_position += s16_to_float(sbo.y_pos_data[buffer_offset + (gl_VertexID / 2)], is_right_channel);		
			buffer_offset += buffer_size;
		}
	} else {
		for (int i = 0; i < num_buffers; i++) {
			y_position += intBitsToFloat(sbo.y_pos_data[buffer_offset + gl_VertexID]);
			buffer_offset += buffer_size;
		}
	}
	y_position /= num_buffers;

	const float x_pos = float(gl_VertexID & (~1)) / float(size_frames) - 1.0f; // == float(gl_VertexID & (~1)) * 0.5f / float(size_frames)) * 2.0f - 1.0f;;
	
	
	const float y_offset = 0.25f - 0.5f * float(is_right_channel);
	const float y_scale = WF_SCALE;
	const float y_pos = y_position * y_scale + y_offset;
	
	const float amp = abs(y_position);
	vs_out.colour = vec3(amp, 0.0f, (1.0f - amp));
	
    gl_Position = vec4(x_pos, y_pos, y_offset, 1.0f); // z coord just holds the value for geometry shader
}