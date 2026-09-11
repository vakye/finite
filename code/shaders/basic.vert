#version 450

struct vertex
{
    vec2 Position;
    vec2 TexCoord;
    vec4 Color;
};

layout(binding = 0) readonly buffer VertexBuffer
{
    vertex Vertices[];
};

layout(push_constant) uniform ShaderGlobals
{
    mat4 Projection;
} Globals;

layout(location = 0) out VertexShaderOut
{
    vec2 TexCoord;
    vec4 Color;
} Out;

void main()
{
    vertex Vertex = Vertices[gl_VertexIndex];

    gl_Position     = Globals.Projection * vec4(Vertex.Position, 0.0, 1.0);
    Out.TexCoord    = Vertex.TexCoord;
    Out.Color       = Vertex.Color;
}

