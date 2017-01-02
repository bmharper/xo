#pragma once

#include "Base/PlatformDefine.h"
#include "Base/Base.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4345) // POD initialized with ()
#endif

#include "Defs.h"
#include "Doc.h"
#include "DocGroup.h"
#include "Dom/DomCanvas.h"
#include "Canvas/Canvas2D.h"
#include "Layout/Layout.h"
#include "Render/Renderer.h"
#include "Render/RenderDoc.h"
#include "Image/ImageStore.h"
#include "Image/Image.h"
#include "SysWnd.h"
#include "Event.h"
#include "Controls/EditBox.h"

#ifdef _WIN32
#pragma warning(pop)
#endif
