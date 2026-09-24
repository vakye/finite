
// NOTE(vak): Forward declarations for functionality that are
// implemented by the underlying platform (linux, windows, ...)

#pragma once

typedef ssize time64;

static time64   GetTimestamp(void);
static time64   SecondsToTimestamp(f64 Seconds);
static f64      TimestampToSeconds(time64 Timestamp);
static f64      GetSecondsElapsed(time64 From, time64 To);

static usize WriteStdOut(void* Data, usize Size, void* Ignore);
static usize WriteStdErr(void* Data, usize Size, void* Ignore);
static usize PrintOut(string String);
static usize PrintErr(string String);

