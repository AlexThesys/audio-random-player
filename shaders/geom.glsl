// geometry shader
#version 440 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

const float width = 1.0f / 256.0f;
const vec4 width4 = vec4(width, 0.0f, 0.0f, 0.0f);

void main()
{

    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + width4; 
    EmitVertex();
    const vec4 vert = vec4(gl_in[0].gl_Position.x, 0.0f, 0.0f, 1.0f);
    gl_Position = vert;
    EmitVertex();
    gl_Position = vert + width4; 
    EmitVertex();

    EndPrimitive();
}