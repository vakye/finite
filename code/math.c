
#pragma once

typedef union
{
    struct { f32 X, Y; };
    struct { f32 R, G; };
    struct { f32 U, V; };
    struct { f32 E[2]; };
} v2;

static v2 V2Zero                (void)                      { return (v2){0}; }
static v2 V2                    (f32 X, f32 Y)              { return (v2){.E = {X, Y}}; }
static v2 V2Scalar              (f32 V)                     { return (v2){.E = {V, V}}; }

static v2 V2Negate              (v2 A)                      { return (v2){.E = {-A.X, -A.Y}}; }

static v2 V2Add                 (v2 A, v2 B)                { return (v2){.E = {A.X + B.X, A.Y + B.Y}}; }
static v2 V2Sub                 (v2 A, v2 B)                { return (v2){.E = {A.X - B.X, A.Y - B.Y}}; }
static v2 V2Mul                 (v2 A, v2 B)                { return (v2){.E = {A.X * B.X, A.Y * B.Y}}; }
static v2 V2Div                 (v2 A, v2 B)                { return (v2){.E = {A.X / B.X, A.Y / B.Y}}; }

static v2 V2AddScalar           (v2 A, f32 B)               { return (v2){.E = {A.X + B, A.Y + B}}; }
static v2 V2SubScalar           (v2 A, f32 B)               { return (v2){.E = {A.X - B, A.Y - B}}; }
static v2 V2MulScalar           (v2 A, f32 B)               { return (v2){.E = {A.X * B, A.Y * B}}; }
static v2 V2DivScalar           (v2 A, f32 B)               { return (v2){.E = {A.X / B, A.Y / B}}; }

static v2 V2ScalarAdd           (f32 A, v2 B)               { return (v2){.E = {A + B.X, A + B.Y}}; }
static v2 V2ScalarSub           (f32 A, v2 B)               { return (v2){.E = {A - B.X, A - B.Y}}; }
static v2 V2ScalarMul           (f32 A, v2 B)               { return (v2){.E = {A * B.X, A * B.Y}}; }
static v2 V2ScalarDiv           (f32 A, v2 B)               { return (v2){.E = {A / B.X, A / B.Y}}; }

static f32 V2Dot                (v2 A, v2 B)                { return (A.X*B.X + A.Y*B.Y); }
static f32 V2LengthSq           (v2 A)                      { return V2Dot(A, A); }
static f32 V2Length             (v2 A)                      { return SquareRoot(V2Dot(A, A)); }
static f32 V2InvLength          (v2 A)                      { return InvSquareRoot(V2Dot(A, A)); }

static v2 V2Normalize           (v2 A)                      { return V2MulScalar(A, V2InvLength(A)); }
static v2 V2NormalizeOrZero     (v2 A)                      { if (V2LengthSq(A) > 1e-14) { return V2Normalize(A); } else { return V2Zero(); } }

typedef union
{
    struct { f32 X, Y, Z; };
    struct { f32 R, G, B; };
    struct { f32 U, V, W; };
    struct { f32 E[3]; };
} v3;

static v3 V3Zero                (void)                          { return (v3){0}; }
static v3 V3                    (f32 X, f32 Y, f32 Z)           { return (v3){.E = {X, Y, Z}}; }
static v3 V3Scalar              (f32 V)                         { return (v3){.E = {V, V, V}}; }

static v3 V3Negate              (v3 A)                          { return (v3){.E = {-A.X, -A.Y, -A.Z}}; }

static v3 V3Add                 (v3 A, v3 B)                    { return (v3){.E = {A.X + B.X, A.Y + B.Y, A.Z + B.Z}}; }
static v3 V3Sub                 (v3 A, v3 B)                    { return (v3){.E = {A.X - B.X, A.Y - B.Y, A.Z - B.Z}}; }
static v3 V3Mul                 (v3 A, v3 B)                    { return (v3){.E = {A.X * B.X, A.Y * B.Y, A.Z * B.Z}}; }
static v3 V3Div                 (v3 A, v3 B)                    { return (v3){.E = {A.X / B.X, A.Y / B.Y, A.Z / B.Z}}; }

static v3 V3AddScalar           (v3 A, f32 B)                   { return (v3){.E = {A.X + B, A.Y + B, A.Z + B}}; }
static v3 V3SubScalar           (v3 A, f32 B)                   { return (v3){.E = {A.X - B, A.Y - B, A.Z - B}}; }
static v3 V3MulScalar           (v3 A, f32 B)                   { return (v3){.E = {A.X * B, A.Y * B, A.Z * B}}; }
static v3 V3DivScalar           (v3 A, f32 B)                   { return (v3){.E = {A.X / B, A.Y / B, A.Z / B}}; }

static v3 V3ScalarAdd           (f32 A, v3 B)                   { return (v3){.E = {A + B.X, A + B.Y, A + B.Z}}; }
static v3 V3ScalarSub           (f32 A, v3 B)                   { return (v3){.E = {A - B.X, A - B.Y, A - B.Z}}; }
static v3 V3ScalarMul           (f32 A, v3 B)                   { return (v3){.E = {A * B.X, A * B.Y, A * B.Z}}; }
static v3 V3ScalarDiv           (f32 A, v3 B)                   { return (v3){.E = {A / B.X, A / B.Y, A / B.Z}}; }

static f32 V3Dot                (v3 A, v3 B)                    { return (A.X*B.X + A.Y*B.Y + A.Z*B.Z); }
static f32 V3LengthSq           (v3 A)                          { return V3Dot(A, A); }
static f32 V3Length             (v3 A)                          { return SquareRoot(V3Dot(A, A)); }
static f32 V3InvLength          (v3 A)                          { return InvSquareRoot(V3Dot(A, A)); }

static v3 V3Normalize           (v3 A)                          { return V3MulScalar(A, V3InvLength(A)); }
static v3 V3NormalizeOrZero     (v3 A)                          { if (V3LengthSq(A) > 1e-14) { return V3Normalize(A); } else { return V3Zero(); } }

typedef union
{
    struct { f32 X, Y, Z, W; };
    struct { f32 R, G, B, A; };
    struct { f32 E[4]; };
} v4;

static v4 V4Zero                (void)                          { return (v4){0}; }
static v4 V4                    (f32 X, f32 Y, f32 Z, f32 W)    { return (v4){.E = {X, Y, Z, W}}; }
static v4 V4Scalar              (f32 V)                         { return (v4){.E = {V, V, V, V}}; }

static v4 V4Negate              (v4 A)                          { return (v4){.E = {-A.X, -A.Y, -A.Z, -A.W}}; }

static v4 V4Add                 (v4 A, v4 B)                    { return (v4){.E = {A.X + B.X, A.Y + B.Y, A.Z + B.Z, A.W + B.W}}; }
static v4 V4Sub                 (v4 A, v4 B)                    { return (v4){.E = {A.X - B.X, A.Y - B.Y, A.Z - B.Z, A.W - B.W}}; }
static v4 V4Mul                 (v4 A, v4 B)                    { return (v4){.E = {A.X * B.X, A.Y * B.Y, A.Z * B.Z, A.W * B.W}}; }
static v4 V4Div                 (v4 A, v4 B)                    { return (v4){.E = {A.X / B.X, A.Y / B.Y, A.Z / B.Z, A.W / B.W}}; }

static v4 V4AddScalar           (v4 A, f32 B)                   { return (v4){.E = {A.X + B, A.Y + B, A.Z + B, A.W + B}}; }
static v4 V4SubScalar           (v4 A, f32 B)                   { return (v4){.E = {A.X - B, A.Y - B, A.Z - B, A.W - B}}; }
static v4 V4MulScalar           (v4 A, f32 B)                   { return (v4){.E = {A.X * B, A.Y * B, A.Z * B, A.W * B}}; }
static v4 V4DivScalar           (v4 A, f32 B)                   { return (v4){.E = {A.X / B, A.Y / B, A.Z / B, A.W / B}}; }

static v4 V4ScalarAdd           (f32 A, v4 B)                   { return (v4){.E = {A + B.X, A + B.Y, A + B.Z, A + B.W}}; }
static v4 V4ScalarSub           (f32 A, v4 B)                   { return (v4){.E = {A - B.X, A - B.Y, A - B.Z, A - B.W}}; }
static v4 V4ScalarMul           (f32 A, v4 B)                   { return (v4){.E = {A * B.X, A * B.Y, A * B.Z, A * B.W}}; }
static v4 V4ScalarDiv           (f32 A, v4 B)                   { return (v4){.E = {A / B.X, A / B.Y, A / B.Z, A / B.W}}; }

static f32 V4Dot                (v4 A, v4 B)                    { return (A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W); }
static f32 V4LengthSq           (v4 A)                          { return V4Dot(A, A); }
static f32 V4Length             (v4 A)                          { return SquareRoot(V4Dot(A, A)); }
static f32 V4InvLength          (v4 A)                          { return InvSquareRoot(V4Dot(A, A)); }

static v4 V4Normalize           (v4 A)                          { return V4MulScalar(A, V4InvLength(A)); }
static v4 V4NormalizeOrZero     (v4 A)                          { if (V4LengthSq(A) > 1e-14) { return V4Normalize(A); } else { return V4Zero(); } }
            
typedef struct
{
    v2 Min;
    v2 Max;
} rect2;

static rect2 R2MinMax(v2 Min, v2 Max)
{
    return (rect2){Min, Max};
}

static rect2 R2MinSize(v2 Min, v2 Size)
{
    return (rect2){Min, V2Add(Min, Size)};
}

static rect2 R2CenterSize(v2 Center, v2 Size)
{
    v2 HalfSize = V2MulScalar(Size, 0.5f);
    return (rect2){V2Sub(Center, HalfSize), V2Add(Center, HalfSize)};
}

static v2 R2GetCenter(rect2 Rect)
{
    v2 Result = V2ScalarMul(0.5f, V2Add(Rect.Min, Rect.Max));
    return (Result);
}

static v2 R2GetSize(rect2 Rect)
{
    v2 Result = V2Sub(Rect.Max, Rect.Min);
    return (Result);
}

static rect2 R2Expand(rect2 Rect, v2 Apron)
{
    rect2 Expanded = R2MinMax(
        V2Sub(Rect.Min, Apron),
        V2Add(Rect.Max, Apron)
    );

    return (Expanded);
}

static b32 R2Intersects(rect2 A, rect2 B)
{
    b32 IsOutside =
        (A.Min.X > B.Max.X) ||
        (A.Max.X < B.Min.X) ||
        (A.Min.Y > B.Max.Y) ||
        (A.Max.Y < B.Min.Y);

    b32 Result = !IsOutside;
    return (Result);
}

typedef struct
{
    f32 E[16];
} m4x4;

static m4x4 M4x4Identity(void)
{
    m4x4 Result = {.E = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    }};

    return (Result);
}

static m4x4 M4x4Orthographic2D(rect2 ViewRect)
{
    v2 ViewCenter   = R2GetCenter(ViewRect);
    v2 ViewSize     = R2GetSize(ViewRect);
    v2 Scale        = V2ScalarDiv(2.0f, ViewSize);
    v2 Translate    = V2Mul(Scale, V2Negate(ViewCenter));

    m4x4 Result = {.E = {
        Scale.X,        0.0f,           0.0f,           0.0f,
        0.0f,           Scale.Y,        0.0f,           0.0f,
        0.0f,           0.0f,           1.0f,           0.0f,
        Translate.X,    Translate.Y,    0.0f,           1.0f,
    }};

    return (Result);
}

static v4 M4x4MultiplyV4(m4x4 A, v4 B)
{
    v4 Result = V4(
        (A.E[0] * B.X) + (A.E[4] * B.Y) + (A.E[8]  * B.Z) + (A.E[12] * B.W),
        (A.E[1] * B.X) + (A.E[5] * B.Y) + (A.E[9]  * B.Z) + (A.E[13] * B.W),
        (A.E[2] * B.X) + (A.E[6] * B.Y) + (A.E[10] * B.Z) + (A.E[14] * B.W),
        (A.E[3] * B.X) + (A.E[7] * B.Y) + (A.E[11] * B.Z) + (A.E[15] * B.W)
    );

    return (Result);
}

