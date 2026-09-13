
#pragma once

typedef struct
{
    unsigned long long State;
} random_state;

static unsigned int RandomU32(random_state* Entropy)
{
    unsigned long long X = Entropy->State;
    unsigned int Count = (unsigned int)(X >> 59);

    Entropy->State = X * 6364136223846793005u + 1442695040888963407u;
    X ^= (X >> 18);

    unsigned int Result = RotateRight32((unsigned int)(X >> 27), Count);
    return (Result);
}

static float RandomUnilateral(random_state* Entropy)
{
    float Result = (float)RandomU32(Entropy) / (float)U32Max;
    return (Result);
}

static float RandomBilateral(random_state* Entropy)
{
    float Result = -1.0f + 2.0f*RandomUnilateral(Entropy);
    return (Result);
}

