#version 450

layout(location = 0) in VertexShaderOut
{
    vec2    TexCoord;
    vec4    Color;
    float   TextureIndex;
} In;

// NOTE(vak): Will be configured to GameTexture_COUNT by Vulkan backend when creating pipeline layout
layout(constant_id = 0) const int TextureCount = 2; 

layout(binding = 1) uniform sampler2D TextureSamplers[TextureCount];

layout(location = 0) out vec4 FragmentColor;

void main()
{
    FragmentColor = texture(TextureSamplers[uint(In.TextureIndex)], In.TexCoord) * In.Color;
}

