
#pragma once

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
    // NOTE(vak): An IEEE754 floating point number X can be decomposed into
    //      X = 2^Exponent * Mantissa
    //
    // Thus, the square root of X is
    //      sqrt(X) = sqrt(2^Exponent * Mantissa)
    //              = sqrt(2^Exponent) * sqrt(Mantissa)
    //              = 2^(Exponent/2) * sqrt(Mantissa)
    //
    // The exponent can simply be extraced and divided by 2. Then, the mantissa
    // is extraced. Floating point mantissas belong in the interval [1, 2), so
    // four iterations of the Newton method is enough to converge to a satisfactory
    // result.

    // Note that odd exponents are multiplied by an additional sqrt(2), which is 2^0.5
    // to obtain the correct result.

    union
    {
        u32 U32;
        f32 F32;
    } Value = {.F32 = X};

    // NOTE(vak): Extract and divide exponent by 2

    ssize Exponent      = (ssize)((Value.U32 >> 23) & 0xFF) - 127;
    ssize SqrtExponent  = Exponent / 2;
    float Multiplier    = (Exponent & 1) ? (1.4142135623730950488f) : (1.0f);

    // NOTE(vak): Extract mantissa and perform four of the Newton method

    Value.U32 &= ~(0xFF << 23);
    Value.U32 |=  (127  << 23);

    f32 SqrtMantissa = Value.F32;

    SqrtMantissa = 0.5f * (SqrtMantissa + (Value.F32 / SqrtMantissa));
    SqrtMantissa = 0.5f * (SqrtMantissa + (Value.F32 / SqrtMantissa));
    SqrtMantissa = 0.5f * (SqrtMantissa + (Value.F32 / SqrtMantissa));
    SqrtMantissa = 0.5f * (SqrtMantissa + (Value.F32 / SqrtMantissa));

    // NOTE(vak): Construct result from SqrtExponent and SqrtMantissa

    union
    {
        u32 U32;
        f32 F32;
    } Result = {0};

    Result.U32 |= ((SqrtExponent + 127) << 23);
    Result.F32 *= SqrtMantissa * Multiplier;

    return (Result.F32);
}

static f32 InvSquareRoot(f32 X)
{
    f32 Result = 1.0f/SquareRoot(X);
    return (Result);
}

