#version 450

vec2 Positions[3] =
{
    vec2(-0.5, -0.5),
    vec2(+0.5, -0.5),
    vec2(+0.0, +0.5),
};

vec4 Colors[3] =
{
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0),
};

layout(location = 0) out VertexShaderOut
{
    vec4 Color;
} Out;

void main()
{
    gl_Position = vec4(Positions[gl_VertexIndex], 0.0, 1.0);
    Out.Color   = Colors[gl_VertexIndex];
}

