
// NOTE(vak): Linux implementation of platform.c

#pragma once

static usize WriteStdOut(void* Data, usize Size, void* Ignore)
{
    ssize Written = write(STDOUT_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

static usize WriteStdErr(void* Data, usize Size, void* Ignore)
{
    ssize Written = write(STDERR_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

static usize PrintOut(string String)
{
    usize Result = WriteStdOut(String.Data, String.Size, 0);
    return (Result);
}

static usize PrintErr(string String)
{
    usize Result = WriteStdErr(String.Data, String.Size, 0);
    return (Result);
}

