
#pragma once

#include <immintrin.h>

static u32 RotateRight32(u32 Value, u32 Shift)
{
	u32 Result = (Value >> Shift) | (Value << (-Shift & 31));
    return (Result);
}

static f32 Square(f32 X)
{
    f32 Result = X*X;
    return (Result);
}

static f32 SquareRoot(f32 X)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(X)));
    return (Result);
}

static f32 InvSquareRoot(f32 X)
{
    f32 Result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(X)));
    return (Result);
}

