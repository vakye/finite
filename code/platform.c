
// NOTE(vak): Forward declarations for functionality that are
// implemented by the underlying platform (linux, windows, ...)

#pragma once

static usize WriteStdOut(void* Data, usize Size, void* Ignore);
static usize WriteStdErr(void* Data, usize Size, void* Ignore);
static usize PrintOut(string String);
static usize PrintErr(string String);

