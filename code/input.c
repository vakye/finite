
#pragma once

// NOTE(vak): Cheatsheet

typedef enum
{
    InputButton_MoveLeft,
    InputButton_MoveRight,
    InputButton_MoveUp,
    InputButton_MoveDown,
    InputButton_Shoot,

    InputButton_COUNT,
} input_button;

static void InputPrepareForFrame    (void);
static void InputReportButton       (input_button Button, b32 IsDown);

static b32  InputIsButtonDown       (input_button Button);
static b32  InputIsButtonUp         (input_button Button);
static b32  InputIsButtonPressed    (input_button Button);
static b32  InputIsButtonReleased   (input_button Button);

// NOTE(vak): Implementation

typedef struct
{
    b32 IsDown;     // NOTE(vak): This frame
    b32 WasDown;    // NOTE(vak): Last frame
} input_button_state;

typedef struct
{
    input_button_state ButtonStates[InputButton_COUNT];
} input_state;

static input_state Input = {0};

static void InputPrepareForFrame(void)
{
    for (usize Index = 0; Index < ArrayCount(Input.ButtonStates); Index++)
    {
        input_button_state* State = Input.ButtonStates + Index;
        State->WasDown = State->IsDown;
    }
}

static void InputReportButton(input_button Button, b32 IsDown)
{
    input_button_state* State = Input.ButtonStates + Button;
    State->IsDown = IsDown;
}

static b32 InputIsButtonDown(input_button Button)
{
    input_button_state* State = Input.ButtonStates + Button;
    b32 Result = State->IsDown;
    return (Result);
}

static b32 InputIsButtonUp(input_button Button)
{
    input_button_state* State = Input.ButtonStates + Button;
    b32 Result = (!State->IsDown);
    return (Result);
}

static b32 InputIsButtonPressed(input_button Button)
{
    input_button_state* State = Input.ButtonStates + Button;
    b32 Result = (State->IsDown) && (!State->WasDown);
    return (Result);
}

static b32 InputIsButtonReleased(input_button Button)
{
    input_button_state* State = Input.ButtonStates + Button;
    b32 Result = (!State->IsDown) && (State->WasDown);
    return (Result);
}

