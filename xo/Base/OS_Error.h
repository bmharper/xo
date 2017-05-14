#pragma once

namespace xo {

extern XO_API StaticError ErrEACCESS;
extern XO_API StaticError ErrEEXIST;
extern XO_API StaticError ErrEINVAL;
extern XO_API StaticError ErrEMFILE;
extern XO_API StaticError ErrENOENT;

#ifdef _WIN32
XO_API Error ErrorFrom_GetLastError();
XO_API Error ErrorFrom_GetLastError(DWORD err);
#endif
} // namespace xo
