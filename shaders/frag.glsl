// fragment shader
# version 440 core

layout (location=0) out vec4 frag_colour;

in g_colour {
    vec3 colour;
}	fs_in;

void main()
{
    frag_colour = vec4(fs_in.colour, 1.0f);
}