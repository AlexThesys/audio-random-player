// vertex shader
#version 440 core

#define FRAMES_PER_BUFFER 0x100 // same as in constants.h

#define WF_SCALE 0.5f

#define s16_min 32768.0f
#define s16_max 32767.0f
#define conversion_factor (2.0f / (s16_max + s16_min))
#define s16_mask 0x0000FFFF
#define s16_bits 0x10
#define s16_sign_bit 0x8000

layout (std430, binding = 0) buffer storage_block {
	int y_pos_data[]; // can accept both s16 and floats
} sbo;

layout (std140, binding = 1) uniform ubo_block {
	int width;
	int fp_mode;
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
	const int size = FRAMES_PER_BUFFER;
	
	const int is_right_channel = gl_VertexID & 0x01;
	
	const float y_position = (ubo.fp_mode == 0) ? s16_to_float(sbo.y_pos_data[gl_VertexID / 2], is_right_channel) : intBitsToFloat(sbo.y_pos_data[gl_VertexID]);

	const float x_pos = float(gl_VertexID & (~1)) / float(size) - 1.0f; // == float(gl_VertexID & (~1)) * 0.5f / float(size)) * 2.0f - 1.0f;;
	
	
	const float offset = 0.25f - 0.5f * float(is_right_channel);
	const float scale = WF_SCALE;
	const float y_pos = y_position * scale + offset;
	
	const float amp = abs(y_position);
	vs_out.colour = vec3(amp, 0.0f, (1.0f - amp));
	
    gl_Position = vec4(x_pos, y_pos, offset, 1.0f); // z coord just holds the value for geometry shader
}