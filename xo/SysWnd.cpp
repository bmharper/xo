#include "pch.h"
#include "SysWnd.h"
#include "DocGroup.h"
#include "Doc.h"
#include "Render/RenderGL.h"
#include "Render/RenderDX.h"

#if XO_PLATFORM_WIN_DESKTOP
#include "SysWnd_windows.h"
#elif XO_PLATFORM_ANDROID
#include "SysWnd_android.h"
#elif XO_PLATFORM_LINUX_DESKTOP
#include "SysWnd_linux.h"
#endif

namespace xo {

int64_t SysWnd::NumWindowsCreated = 0;

SysWnd* SysWnd::New() {
#if XO_PLATFORM_WIN_DESKTOP
	return new SysWndWindows();
#elif XO_PLATFORM_ANDROID
	return new SysWndAndroid();
#elif XO_PLATFORM_LINUX_DESKTOP
	return new SysWndLinux();
#endif
}

void SysWnd::PlatformInitialize(const InitParams* init) {
}

SysWnd::SysWnd() {
	InvalidRect.SetInverted();
	NumWindowsCreated++;
}

SysWnd::~SysWnd() {
	if (DocGroup) {
		Global()->DocRemoveQueue.Add(DocGroup);
		DocGroup = nullptr;
	}

	// For a simple, single-window application, by the time we get here, the message loop has already exited, so
	// the application can be confident that it can clean up resources by now, without worrying that it's still
	// going to be receiving UI input during that time. See RunAppLowLevel() and RunApp() to understand how
	// this works.
	SendEvent(SysWnd::EvDestroy);
}

Error SysWnd::CreateWithDoc(uint32_t createFlags) {
	DocGroup      = xo::DocGroup::New();
	DocGroup->Wnd = this;
	auto err      = Create(createFlags);
	if (!err.OK()) {
		delete DocGroup;
		DocGroup = nullptr;
		return err;
	}
	Attach(new xo::Doc(DocGroup), true);
	Global()->DocAddQueue.Add(DocGroup);
	return Error();
}

void SysWnd::Show() {
}

void SysWnd::SetTitle(const char* title) {
}

Doc* SysWnd::Doc() {
	return DocGroup->Doc;
}

void SysWnd::Attach(xo::Doc* doc, bool destroyDocWithGroup) {
	DocGroup->Doc                 = doc;
	DocGroup->DestroyDocWithGroup = destroyDocWithGroup;
}

bool SysWnd::BeginRender() {
	if (Renderer)
		return Renderer->BeginRender(*this);
	else
		return false;
}

void SysWnd::EndRender(uint32_t endRenderFlags) {
	if (Renderer) {
		XOTRACE_LATENCY("EndRender (begin) %s\n", !!(endRenderFlags & EndRenderNoSwap) ? "noswap" : "swap");
		Renderer->EndRender(*this, endRenderFlags);
		XOTRACE_LATENCY("EndRender (end)\n");
	}
}

void SysWnd::SurfaceLost() {
	if (Renderer)
		Renderer->SurfaceLost();
}

void SysWnd::SendEvent(Event ev) {
	for (auto cb : EventListeners)
		cb(ev);
}

void SysWnd::SetPosition(Box box, uint32_t setPosFlags) {
	Trace("SysWnd.SetPosition is not implemented\n");
}

void SysWnd::PostCursorChangedMessage() {
}

void SysWnd::PostRepaintMessage() {
}

bool SysWnd::CopySurfaceToImage(Box box, Image& img) {
	return false;
}

void SysWnd::AddToSystemTray(const char* title, bool hideInsteadOfClose) {
}

void SysWnd::ShowSystemTrayAlert(const char* msg) {
}

void SysWnd::InvalidateRect(Box box) {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	InvalidRect.ExpandToFit(box);
}

Box SysWnd::GetInvalidateRect() {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	return InvalidRect;
}

void SysWnd::ValidateWindow() {
	std::lock_guard<std::mutex> lock(InvalidRect_Lock);
	InvalidRect.SetInverted();
}

Error SysWnd::InitializeRenderer() {
#if XO_PLATFORM_WIN_DESKTOP
	Error err;
	if (Global()->PreferOpenGL) {
		err = InitializeRenderer_Any<RenderGL>(Renderer);
		if (err.OK())
			return Error();
		err = InitializeRenderer_Any<RenderDX>(Renderer);
		if (err.OK())
			return Error();
	} else {
		err = InitializeRenderer_Any<RenderDX>(Renderer);
		if (err.OK())
			return Error();
		err = InitializeRenderer_Any<RenderGL>(Renderer);
		if (err.OK())
			return Error();
	}
	return err;
#else
	return InitializeRenderer_Any<RenderGL>(Renderer);
#endif
}

template <typename TRenderer>
Error SysWnd::InitializeRenderer_Any(RenderBase*& renderer) {
	renderer = new TRenderer();
	if (renderer->InitializeDevice(*this)) {
		Trace("Successfully initialized %s renderer\n", renderer->RendererName());
		return Error();
	} else {
		Trace("Failed to initialize %s renderer\n", renderer->RendererName());
		delete renderer;
		renderer = NULL;
		return Error::Fmt("Failed to initialize %v renderer", renderer->RendererName());
	}
}
} // namespace xo
