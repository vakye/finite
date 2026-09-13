
#pragma once

#include <immintrin.h>

static unsigned int RotateRight32(unsigned int Value, unsigned int Shift)
{
	unsigned int Result = (Value >> Shift) | (Value << (-Shift & 31));
    return (Result);
}

static float Square(float X)
{
    float Result = X*X;
    return (Result);
}

static float SquareRoot(float X)
{
    float Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(X)));
    return (Result);
}

static float InvSquareRoot(float X)
{
    float Result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(X)));
    return (Result);
}

