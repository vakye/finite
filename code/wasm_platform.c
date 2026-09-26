
// NOTE(vak): WASM implementation of platform.c

#pragma once

static time64 GetTimestamp(void)
{
    time64 Result = (time64)(JS_MillisecondsNow() * 1e6);
    return (Result);
}

static time64 SecondsToTimestamp(f64 Seconds)
{
    time64 Result = (time64)(Seconds * 1e9);
    return (Result);
}

static f64 TimestampToSeconds(time64 Timestamp)
{
    f64 Result = Timestamp * 1e-9;
    return (Result);
}

static f64 GetSecondsElapsed(time64 From, time64 To)
{
    f64 Result = TimestampToSeconds(To - From);
    return (Result);
}

