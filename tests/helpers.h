#pragma once

inline bool EQ(const char* a, const char* b) { return strcmp(a, b) == 0; }

xo::String LoadFileAsString(const char* file);
