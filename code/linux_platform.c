
// NOTE(vak): Linux implementation of platform.c

#pragma once

static time64 GetTimestamp(void)
{
    struct timespec Now = {0};
    clock_gettime(CLOCK_MONOTONIC, &Now);

    time64 Result =
        ((ssize)(Now.tv_sec  & 0xFFFFFFFF) << 32) |
        ((ssize)(Now.tv_nsec & 0xFFFFFFFF) <<  0);

    return (Result);
}

static time64 SecondsToTimestamp(f64 Seconds)
{
    usize IntegerPart = (usize)Seconds;
    f64 DecimalPart = Seconds - IntegerPart;
    f64 Nanoseconds = DecimalPart * 1e9;

    time64 Result =
        ((ssize)Seconds     << 32) |
        ((ssize)Nanoseconds <<  0);

    return (Result);
}

static f64 TimestampToSeconds(time64 Timestamp)
{
    f64 Result = (f64)(Timestamp >> 32) + (1e-9 * (f64)(Timestamp & 0xFFFFFFFF));
    return (Result);
}

static f64 GetSecondsElapsed(time64 From, time64 To)
{
    f64 Result = TimestampToSeconds(To - From);
    return (Result);
}

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

