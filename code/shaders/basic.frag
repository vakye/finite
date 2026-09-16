#version 450

layout(location = 0) in VertexShaderOut
{
    vec2 TexCoord;
    vec4 Color;
} In;

layout(binding = 1) uniform sampler2D TextureSampler;

layout(location = 0) out vec4 FragmentColor;

void main()
{
    FragmentColor = texture(TextureSampler, In.TexCoord) * In.Color;
}

