
#pragma once

// NOTE(vak): Cheatsheet

typedef enum
{
    // NOTE(vak): Let f(t) : [0, 1] -> R be a map from some
    // percentage t to the computed interpolation percentage.
    //
    // f(t) is subject to the following constraints:
    //          f(0) = 0
    //          f(1) = 1
    //
    // Then, the update step is formulated as:
    //
    //          Varying = Last + (Now - Last)*f(t)
    //

    SmoothKind_Linear = 0,  // NOTE(vak): f(t) = t
    SmoothKind_Squared,     // NOTE(vak): f(t) = t^2
    SmoothKind_Cubed,       // NOTE(vak): f(t) = t^3
} smooth_kind;

typedef struct
{
    smooth_kind Kind;       // NOTE(vak): What kind of animation to update 'Varying' with

    time64 Latency;         // NOTE(vak): Time it takes for 'Varying' to move from 'Last' to 'Now'
    time64 LastSetTime;     // NOTE(vak): Last time when 'Last' and 'Now' were updated

    f32 Now;                // NOTE(vak): Target value that 'Varying' is moving towards
    f32 Last;               // NOTE(vak): Value of 'Varying' when 'Now' was last set
    f32 Varying;            // NOTE(vak): Value that moves in a smooth manner from 'Last' to 'Now'
} smooth_f32;

static smooth_f32   SmoothF32(smooth_kind Kind, f32 LatencySeconds, f32 InitialValue);
static void         SetSmoothF32(smooth_f32* Value, f32 TargetValue);
static void         UpdateSmoothF32(smooth_f32* Value);

// NOTE(vak): Implementation

static smooth_f32 SmoothF32(smooth_kind Kind, f32 LatencySeconds, f32 InitialValue)
{
    smooth_f32 Result =
    {
        .Kind = Kind,
        .Latency = SecondsToTimestamp(LatencySeconds),
        .Last = InitialValue,
        .Now = InitialValue,
        .Varying = InitialValue,
    };

    return (Result);
}

static void SetSmoothF32(smooth_f32* Value, f32 TargetValue)
{
    Value->Last = Value->Varying;
    Value->Now = TargetValue;

    Value->LastSetTime = GetTimestamp();
}

static void UpdateSmoothF32(smooth_f32* Value)
{
    time64 Elapsed = GetTimestamp() - Value->LastSetTime;
    if (Elapsed >= Value->Latency)
    {
        Value->Varying = Value->Now;
    }
    else
    {
        f32 T = (f32)((f64)Elapsed / (f64)Value->Latency);
        f32 F = 0.0f;

        switch (Value->Kind)
        {
            case SmoothKind_Linear:     F = T;          break;
            case SmoothKind_Squared:    F = T*T;        break;
            case SmoothKind_Cubed:      F = T*T*T;      break;
        }

        Value->Varying = Value->Last + (Value->Now - Value->Last)*F;
    }
}

