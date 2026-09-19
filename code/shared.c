
#pragma once

#include <immintrin.h>

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef s64 ssize;
typedef u64 usize;

typedef float f32;
typedef double f64;

typedef u8 b8;
typedef u32 b32;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Assert(Expression) if (!(Expression)) __builtin_trap()

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Absolute(Value) ((Value) < 0 ? -(Value) : (Value))

#define SafeDivide0(A, B) ((Absolute(B) > 1e-14f) ? ((A) / (B)) : (0))

#define U32Max (~0U)
#define U64Max (~0ULL)

#define true  (1)
#define false (0)

typedef struct
{
    char* Data;
    usize Size;
} string;

#define Str(Literal)        (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

