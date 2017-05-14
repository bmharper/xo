#pragma once

namespace xo {

bool AnyDocsDirty();

// This is implemented by in platform-specific files, for example in MsgLoop_windows.cpp, or MsgLoop_linux.cpp
XO_API void RunMessageLoop();

} // namespace xo
