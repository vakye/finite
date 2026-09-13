
#pragma once

#include <immintrin.h>

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Absolute(Value) ((Value) < 0 ? -(Value) : (Value))

#define U32Max (~0U)

#define true  (1)
#define false (0)

