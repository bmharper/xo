#pragma once

#include "xoPlatformDefine.h"
#include "xoBase_SystemIncludes.h"
#include "xoBase.h"
#include "xoBase_LocalIncludes.h"
#include "xoBase_Vector.h"
#include "xoBase_Fmt.h"
#include "xoString.h"
#include "../dependencies/Panacea/Strings/fmt.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4345 ) // POD initialized with ()
#endif

#include "xoDefs.h"
#include "xoDoc.h"
#include "xoDocGroup.h"
#include "Dom/xoDomCanvas.h"
#include "Canvas/xoCanvas2D.h"
#include "Layout/xoLayout.h"
#include "Layout/xoLayout2.h"
#include "Render/xoRenderer.h"
#include "Render/xoRenderDoc.h"
#include "Image/xoImageStore.h"
#include "Image/xoImage.h"
#include "xoSysWnd.h"
#include "xoEvent.h"
//#include "Shaders/Helpers/xoPreprocessor.h"

#ifdef _WIN32
#pragma warning( pop )
#endif
