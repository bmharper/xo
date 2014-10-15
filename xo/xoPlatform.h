#pragma once

void*		xoMallocOrDie( size_t bytes );
void*		xoReallocOrDie( void* buf, size_t bytes );
xoString	xoDefaultCacheDir();		// This informs xoGlobals()->CacheDir, if it's not specified via xoInitParams
