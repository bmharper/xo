// We need this so that our symbols don't clash with another instance of xxhash.
// This will happen if you link to libxo.so and liblz4.so

// NOTE: Keep this in sync with the macro defined inside xxhash.cpp
#define XXH_NAMESPACE XO_

#include "xxhash.h"