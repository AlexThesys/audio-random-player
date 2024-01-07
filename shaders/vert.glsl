// vertex shader
#version 440 core

#define min_s16 (-32768.0f)
#define s16_mask 0xFFFF
#define s16_bits 0x10
#define u16_max float(0xFFFF)

layout (std430, binding = 0) buffer storage_block {
	int y_pos_data[]; // can accept both s16 and floats
} sbo;

uniform bool fp_mode;
uniform int size;

float s16_to_float(int val_packed)
{
	const int selector = gl_VertexID & 0x01;
	const int val = (val_packed >> (selector * s16_bits)) & s16_mask;
	return ((float(val) - min_s16) / u16_max) * 2.0f - 1.0f;
}

void main()
{
	const float y_position = (!fp_mode) ? s16_to_float(sbo.y_pos_data[gl_VertexID]) : intBitsToFloat(sbo.y_pos_data[gl_VertexID]);
	
    const float x_pos = float(gl_VertexID & (~1)) / float(size) * 2.0f - 1.0f;
	
	const int is_right_channel = gl_VertexID & 0x01;
	const float offset = 0.25f - 0.5f * float(is_right_channel);
	const float scale = 0.5f;
	const float y_pos = y_position * scale + offset;
	
    gl_Position = vec4(x_pos, y_pos, 0.0f, 1.0f);
}