#pragma once

#ifdef _WIN32

/* Retrieve OS info.
ver		500 = W2k. 501 = XP. 600 = Vista. (this is MajorVersion * 100 + MinorVersion).
is64	True for 64-bit OS.
dotNet	0 = none. 11 = (DotNet 1.1). 20 = (DotNet 2.0). 30 = (DotNet 3.0). 35 = (DotNet 3.5).
video	Primary video card driver. Will not write more than 128 characters, and will always write a null terminator.
*/
PAPI void AbcOSWinGetInfo( int* ver, bool* is64, int* dotNet, char* video );

// Returns true if this is a 32-bit process running on 64-bit windows
PAPI bool AbcOSWinIsWow64();

#endif