#pragma once

#include "nuApiDecl.h"
#include "nuBase_SystemIncludes.h"
#include "nuBase.h"
#include "nuBase_LocalIncludes.h"
#include "nuBase_Vector.h"
#include "nuBase_Fmt.h"
#include "nuString.h"
#include "../dependencies/Panacea/Strings/fmt.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4345 ) // POD initialized with ()
#endif

#include "nuDefs.h"
#include "nuDoc.h"
#include "nuDocGroup.h"
#include "Layout/nuLayout.h"
#include "Render/nuRenderer.h"
#include "Render/nuRenderDoc.h"
#include "Image/nuImageStore.h"
#include "Image/nuImage.h"
#include "nuSysWnd.h"
#include "nuEvent.h"
//#include "Shaders/Helpers/nuPreprocessor.h"

#ifdef _WIN32
#pragma warning( pop )
#endif
