#version 450

layout(location = 0) in VertexShaderOut
{
    vec2 TexCoord;
    vec4 Color;
} In;

layout(location = 0) out vec4 FragmentColor;

void main()
{
    FragmentColor = In.Color;
}

