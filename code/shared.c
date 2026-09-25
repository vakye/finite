
#pragma once

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

#define Clamp(Min, Value, Max) Maximum(Min, Minimum(Max, Value))

#define Absolute(Value) ((Value) < 0 ? -(Value) : (Value))

#define SafeDivide0(A, B) ((Absolute(B) > 1e-14f) ? ((A) / (B)) : (0))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

#define U32Max (~0U)
#define U64Max (~0ULL)

#define true  (1)
#define false (0)

#define ZeroType(Pointer) ZeroMemory(Pointer, sizeof(*(Pointer)))
#define ZeroArray(Pointer, Count) ZeroMemory(Pointer, sizeof(*(Pointer)) * (Count))

static void ZeroMemory(void* DestInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    while (Size--) *Dest++ = 0;
}

static void CopyMemory(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;
    while (Size--) *Dest++ = *Source++;
}

typedef struct
{
    char* Data;
    usize Size;
} string;

#define NilString           (string){0}

#define Str(Literal)        (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

#define StaticStr(Literal)        {Literal, sizeof(Literal) - 1}
#define StaticStrData(Data, Size) {Data, Size}

static b32 IsNilString(string String)
{
    b32 Result = (!(String.Data) || !(String.Size));
    return (Result);
}

static string CString(const char* Data)
{
    string Result = StrData((char*)Data, 0);
    if (Data)
    {
        while (Data[Result.Size] != '\0')
            Result.Size++;
    }

    return (Result);
}

static string StringView(string String, usize From, usize Size)
{
    From = Minimum(From, String.Size);
    Size = Minimum(Size, String.Size - From);

    string Result = StrData(String.Data + From, Size);
    return (Result);
}

static b32 StringEqual(string A, string B)
{
    b32 Result = (A.Size == B.Size);

    for (usize Index = 0; Index < A.Size; Index++)
    {
        if (A.Data[Index] != B.Data[Index])
        {
            Result = false;
            break;
        }
    }

    return (Result);
}

static b32 StringStartsWith(string String, string Match)
{
    b32 Result = (String.Size >= Match.Size);

    for (usize Index = 0; Index < Match.Size; Index++)
    {
        if (String.Data[Index] != Match.Data[Index])
        {
            Result = false;
            break;
        }
    }

    return (Result);
}

