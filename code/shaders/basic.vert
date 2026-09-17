#version 450

struct vertex
{
    float X, Y;
    float U, V;
    float R, G, B, A;
    uint TextureIndex;
};

layout(binding = 0) readonly buffer VertexBuffer
{
    vertex Vertices[];
};

layout(location = 0) out VertexShaderOut
{
    vec2    TexCoord;
    vec4    Color;
    float   TextureIndex;
} Out;

void main()
{
    vertex Vertex = Vertices[gl_VertexIndex];

    vec2 Position       = vec2(Vertex.X, Vertex.Y);
    vec2 TexCoord       = vec2(Vertex.U, Vertex.V);
    vec4 Color          = vec4(Vertex.R, Vertex.G, Vertex.B, Vertex.A);
    uint TextureIndex   = Vertex.TextureIndex;

    gl_Position         = vec4(Position, 0.0, 1.0);
    Out.TexCoord        = TexCoord;
    Out.Color           = Color;
    Out.TextureIndex    = float(TextureIndex);
}

