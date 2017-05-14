#include "pch.h"
#include "SysWnd_android.h"
#include "DocGroup.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"

xo::SysWnd* SingleMainWnd = NULL;

namespace xo {

SysWndAndroid::SysWndAndroid() {
	SingleMainWnd      = this;
	RelativeClientRect = Box(0, 0, 0, 0);
}
SysWndAndroid::~SysWndAndroid() {
	SingleMainWnd = nullptr;
}

Error SysWndAndroid::Create(uint32_t createFlags) {
	return InitializeRenderer();
}

Box SysWndAndroid::GetRelativeClientRect() {
	return RelativeClientRect;
}

} // namespace xo