#pragma once

#include "nuBase.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4345 ) // POD initialized with ()
#endif

#include "nuDefs.h"
#include "nuDoc.h"
#include "nuLayout.h"
#include "Render/nuRenderer.h"
#include "nuSysWnd.h"
#include "nuEvent.h"

#ifdef _WIN32
#pragma warning( pop )
#endif
