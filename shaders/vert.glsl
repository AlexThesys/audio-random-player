// vertex shader
#version 440 core

#define s16_min (-32768.0f)
#define s16_max 32767.0f
#define s16_range (s16_max - s16_min)
#define s16_mask 0xFFFF
#define s16_bits 0x10


layout (std430, binding = 0) buffer storage_block {
	int y_pos_data[]; // can accept both s16 and floats
} sbo;

uniform bool fp_mode;
uniform int size;

float s16_to_float(int val_packed, int selector)
{
	const int val = (val_packed >> (selector * s16_bits)) & s16_mask;
	return ((float(val) - s16_min) / s16_range) * 2.0f - 1.0f;
}

void main()
{
	const float y_position = (!fp_mode) ? s16_to_float(sbo.y_pos_data[gl_VertexID / 2], (gl_VertexID & 0x01)) : intBitsToFloat(sbo.y_pos_data[gl_VertexID]);

	const float x_pos = float(gl_VertexID & (~1)) / float(size) - 1.0f; // == float(gl_VertexID & (~1)) * 0.5f / float(size)) * 2.0f - 1.0f;;
	
	const int is_right_channel = gl_VertexID & 0x01;

	
	const float offset = 0.25f - 0.5f * float(is_right_channel);
	const float scale = 0.5f;
	const float y_pos = y_position * scale + offset;
	
    gl_Position = vec4(x_pos, y_pos, 0.0f, 1.0f);
}