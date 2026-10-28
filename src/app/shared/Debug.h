#pragma once

// Test-only code stays in the release binary only as empty macro expansion.
// Use for friend declarations, test hooks and small DEBUG-only fragments;
// bulk test implementation remains behind the DEBUG include guard in SelfTest.h.
#ifdef DEBUG
#define DebugInfo(...) __VA_ARGS__
#else
#define DebugInfo(...)
#endif
