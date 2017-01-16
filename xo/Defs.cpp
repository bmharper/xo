#include "pch.h"
#include "Defs.h"
#include "DocGroup.h"
#include "Doc.h"
#include "SysWnd.h"
#include "Render/RenderGL.h"
#include "Text/FontStore.h"
#include "Text/GlyphCache.h"

namespace xo {

static int    InitializeCount = 0;
static Style* StaticDefaultTagStyles[TagEND];

// Single globally accessible data
static GlobalStruct* Globals = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Texture::FlipVertical() {
	uint8_t  sline[4096];
	uint8_t* line    = sline;
	size_t   astride = std::abs(TexStride);
	if (astride > sizeof(sline))
		line = (uint8_t*) MallocOrDie(astride);
	for (uint32_t i = 0; i < TexHeight / 2; i++) {
		memcpy(line, TexDataAtLine(TexHeight - i - 1), astride);
		memcpy(TexDataAtLine(TexHeight - i - 1), TexDataAtLine(i), astride);
		memcpy(TexDataAtLine(i), line, astride);
	}
	if (line != sline)
		free(line);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Box::SetInt(int32_t left, int32_t top, int32_t right, int32_t bottom) {
	Left   = RealToPos((float) left);
	Top    = RealToPos((float) top);
	Right  = RealToPos((float) right);
	Bottom = RealToPos((float) bottom);
}

void Box::ExpandToFit(const Box& expando) {
	Left   = Min(Left, expando.Left);
	Top    = Min(Top, expando.Top);
	Right  = Max(Right, expando.Right);
	Bottom = Max(Bottom, expando.Bottom);
}

void Box::ClampTo(const Box& clamp) {
	Left   = Max(Left, clamp.Left);
	Top    = Max(Top, clamp.Top);
	Right  = Min(Right, clamp.Right);
	Bottom = Min(Bottom, clamp.Bottom);
}

Box Box::ShrunkBy(const Box& marginBox) {
	Box c = *this;
	c.Left += marginBox.Left;
	c.Right -= marginBox.Right;
	c.Top += marginBox.Top;
	c.Bottom -= marginBox.Bottom;
	return c;
}

Box Box::PiecewiseSum(const Box& box) {
	Box c = *this;
	c.Left += box.Left;
	c.Right += box.Right;
	c.Top += box.Top;
	c.Bottom += box.Bottom;
	return c;
}

BoxF Box::ToRealBox() const {
	BoxF f;
	f.Left   = PosToReal(Left);
	f.Right  = PosToReal(Right);
	f.Top    = PosToReal(Top);
	f.Bottom = PosToReal(Bottom);
	return f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BoxF Box16::ToRealBox() const {
	BoxF f;
	f.Left   = PosToReal(Left);
	f.Right  = PosToReal(Right);
	f.Top    = PosToReal(Top);
	f.Bottom = PosToReal(Bottom);
	return f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Vec4f Color::GetVec4sRGB() const {
	float s = 1.0f / 255.0f;
	return Vec4f(r * s, g * s, b * s, a * s);
}

Vec4f Color::GetVec4Linear() const {
	float s = 1.0f / 255.0f;
	return Vec4f(SRGB2Linear(r),
	             SRGB2Linear(g),
	             SRGB2Linear(b),
	             a * s);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const float sRGB_Low = 0.0031308f;
static const float sRGB_a   = 0.055f;

XO_API float SRGB2Linear(uint8_t srgb) {
	float g = srgb * (1.0f / 255.0f);
	if (g <= 0.04045)
		return g / 12.92f;
	else
		return pow((g + sRGB_a) / (1.0f + sRGB_a), 2.4f);
}

XO_API uint8_t Linear2SRGB(float linear) {
	float g;
	if (linear <= sRGB_Low)
		g = 12.92f * linear;
	else
		g = (1.0f + sRGB_a) * pow(linear, 1.0f / 2.4f) - sRGB_a;
	return (uint8_t) Round(255.0f * g);
}

void RenderStats::Reset() {
	memset(this, 0, sizeof(*this));
}

// add or remove documents that are queued for addition or removal
XO_API void AddOrRemoveDocsFromGlobalList() {
	DocGroup* p = NULL;

	while (p = Global()->DocRemoveQueue.PopTailR()) {
		size_t pos = Global()->Docs.find(p);
		delete Global()->Docs[pos];
		Global()->Docs.erase(pos);
	}

	while (p = Global()->DocAddQueue.PopTailR())
		Global()->Docs += p;
}

void WorkerThreadFunc() {
	while (true) {
		Global()->JobQueue.SemObj().wait();
		if (Global()->ExitSignalled)
			break;
		Job job;
		XO_VERIFY(Global()->JobQueue.PopTail(job));
		job.JobFunc(job.JobData);
	}
}

void UIThread() {
	while (true) {
		Global()->UIEventQueue.SemObj().wait();
		if (Global()->ExitSignalled)
			break;
		OriginalEvent          ev;
		TQueue<OriginalEvent>& q      = Global()->UIEventQueue;
		uint32_t               qsize1 = q.Size();
		XO_VERIFY(Global()->UIEventQueue.PopTail(ev));
		uint32_t qsize2 = q.Size();
		ev.DocGroup->ProcessEvent(ev.Event);
	}
}

#if XO_PLATFORM_WIN_DESKTOP

static void Initialize_Win32() {
	Global()->UIThread = std::thread(UIThread);
}

static void Shutdown_Win32() {
	if (Global()->UIThread.joinable()) {
		Global()->UIEventQueue.Add(OriginalEvent());
		Global()->UIThread.join();
	}
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Retrieve the one-and-only xo "globals" data
XO_API GlobalStruct* Global() {
	return Globals;
}

static float ComputeEpToPixel() {
#if XO_PLATFORM_WIN_DESKTOP
	float scale = 1;
	HDC   dc    = GetDC(NULL);
	if (dc != NULL) {
		int dpi = GetDeviceCaps(dc, LOGPIXELSX);
		scale   = (float) dpi / 96.0f;
		ReleaseDC(NULL, dc);
	}
	return scale;
#elif XO_PLATFORM_ANDROID
	// Globals->EpToPixel is sent by Java_com_android_xo_XoLib_init when it calls Initialize()
	return 1;
#elif XO_PLATFORM_LINUX_DESKTOP
	// TODO
	return 1;
#else
	XO_TODO_STATIC
#endif
}

static void InitializePlatform() {
#if XO_PLATFORM_WIN_DESKTOP
	SetProcessDPIAware();
#elif XO_PLATFORM_ANDROID
#elif XO_PLATFORM_LINUX_DESKTOP
#else
	XO_TODO_STATIC;
#endif
}

// This must be called once at application startup. It is automatically called by RunApp and RunAppLowLevel.
XO_API void Initialize(const InitParams* init) {
	InitializeCount++;
	if (InitializeCount != 1)
		return;

	InitializePlatform();

	int numCPUCores = GetNumberOfCores();

	Globals                = new GlobalStruct();
	Globals->ExitSignalled = false;

	if (init && init->EpToPixel != 0)
		Globals->EpToPixel = init->EpToPixel;
	else
		Globals->EpToPixel = ComputeEpToPixel();

	// Windows default is 530
	Globals->CaretBlinkTimeMS = 530;
#ifdef XO_PLATFORM_WIN_DESKTOP
	Globals->CaretBlinkTimeMS = GetCaretBlinkTime();
#endif

	if (init && init->CacheDir != "")
		Globals->CacheDir = init->CacheDir;
	else
		Globals->CacheDir = DefaultCacheDir();

	Globals->TargetFPS            = 60;
	Globals->NumWorkerThreads     = Min(numCPUCores, 4); // I can't think of a reason right now why you'd want lots of these
	Globals->MaxSubpixelGlyphSize = 60;
	Globals->PreferOpenGL         = true;
	Globals->EnableVSync          = false;
	// Freetype's output is linear coverage percentage, so if we treat our freetype texture as GL_LUMINANCE
	// (and not GL_SLUMINANCE), and we use an sRGB framebuffer, then we get perfect results without
	// doing any tweaking to the freetype glyphs.
	// Setting SubPixelTextGamma = 2.2 will get results very close to default cleartype on Windows 8.1
	// As far as I know, leaving the gamma at 1.0 is "true to the font designer", but this does leave the fonts
	// quite a bit heavier than the default on Windows. For this reason, we set the gamma to 1.5. It seems like
	// a reasonable blend between the "correct weight" and "prior art".
	// CORRECTION. A gamma of anything other than 1.0 looks bad at small font sizes (like 12 or 13 pixels)
	// We might want to have a "gamma curve" of pixel size vs gamma.
	// FURTHER CORRECTION. The "infinality" patches to Freetype, as well as fixing up our incorrect glyph
	// UV coordinates has made this all redundant.
	Globals->SubPixelTextGamma   = 1.0f;
	Globals->WholePixelTextGamma = 1.0f;
#if XO_PLATFORM_WIN_DESKTOP || XO_PLATFORM_LINUX_DESKTOP
	Globals->EnableSubpixelText    = true;
	Globals->EnableSRGBFramebuffer = true;
//Globals->EmulateGammaBlending = true;
#else
	Globals->EnableSubpixelText    = false;
	Globals->EnableSRGBFramebuffer = false;
//Globals->EmulateGammaBlending = false;
#endif
	// Do we round text line heights to whole pixels?
	// We only render sub-pixel text on low resolution monitors that do not change orientation (ie desktop).
	Globals->RoundLineHeights     = Globals->EnableSubpixelText || Globals->EpToPixel < 2.0f;
	Globals->SnapBoxes            = true;
	Globals->SnapSubpixelHorzText = false;
	Globals->EnableKerning        = !Globals->EnableSubpixelText || !Globals->SnapSubpixelHorzText;
	//Globals->DebugZeroClonedChildList = true;
	Globals->MaxTextureID = ~((TextureID) 0);
	//Globals->ClearColor.Set( 200, 0, 200, 255 );  // Make our clear color a very noticeable purple, so you know when you've screwed up the root node
	Globals->ClearColor.Set(255, 150, 255, 255); // Make our clear color a very noticeable purple, so you know when you've screwed up the root node
	Globals->DocAddQueue.Initialize(false);
	Globals->DocRemoveQueue.Initialize(false);
	Globals->UIEventQueue.Initialize(true);
	Globals->JobQueue.Initialize(true);
	Globals->FontStore = new FontStore();
	Globals->FontStore->InitializeFreetype();
	Globals->GlyphCache = new GlyphCache();
	SysWnd::PlatformInitialize();
#if XO_PLATFORM_WIN_DESKTOP
	Initialize_Win32();
#endif
	Trace("xo creating %d worker threads (%d CPU cores).\n", (int) Globals->NumWorkerThreads, (int) numCPUCores);
	for (int i = 0; i < Globals->NumWorkerThreads; i++)
		Globals->WorkerThreads.push_back(std::thread(WorkerThreadFunc));
}

XO_API void SurfaceLost() {
}

// This is the companion to Initialize.
XO_API void Shutdown() {
	XO_ASSERT(InitializeCount > 0);
	InitializeCount--;
	if (InitializeCount != 0)
		return;

	Globals->ExitSignalled = true;

	for (int i = 0; i < TagEND; i++)
		delete StaticDefaultTagStyles[i];

	// allow documents scheduled for deletion to be deleted
	AddOrRemoveDocsFromGlobalList();

#if XO_PLATFORM_WIN_DESKTOP
	Shutdown_Win32();
#endif

	// signal all threads to exit
	Job nullJob = Job();
	for (size_t i = 0; i < Globals->WorkerThreads.size(); i++)
		Globals->JobQueue.Add(nullJob);

	// wait for each thread in turn
	for (size_t i = 0; i < Globals->WorkerThreads.size(); i++)
		Globals->WorkerThreads[i].join();

	Globals->GlyphCache->Clear();
	delete Globals->GlyphCache;
	Globals->GlyphCache = NULL;

	Globals->FontStore->Clear();
	Globals->FontStore->ShutdownFreetype();
	delete Globals->FontStore;
	Globals->FontStore = NULL;

	delete Globals;
}

/* Use this to launch your application using an API that provides more control than RunApp()
Example:
	// Link to WinMainLowLevel.cpp, or copy the stub code out of that file into your own application.
	#include "../xo/xo.h"

	static SysWnd* MainWnd;

	void Main( MainEvent ev )
	{
		switch ( ev )
		{
		case MainEventInit:
			{
				MainWnd = SysWnd::CreateWithDoc();
				...
				MainWnd->Show();
			}
			break;
		case MainEventShutdown:
			delete MainWnd;
			MainWnd = NULL;
			break;
		}
	}
*/
XO_API void RunAppLowLevel(MainCallbackLowLevel mainCallback) {
	Initialize();
	mainCallback(MainEventInit);
#if XO_PLATFORM_WIN_DESKTOP
	RunWin32MessageLoop();
#elif XO_PLATFORM_LINUX_DESKTOP
	RunXMessageLoop();
#else
	XO_DIE_MSG("RunApp is not supported on this platform");
#endif
	mainCallback(MainEventShutdown);
	Shutdown();
}

/* Use this to create a simple application that doesn't need system events.
Example:
	// Link to WinMain.cpp, or copy the stub code out of that file into your own application.
	#include "../xo/xo.h"

	void Main(SysWnd* wnd)
	{
		wnd->Doc()->Root.AddNode(...
	}
*/
XO_API void RunApp(MainCallback mainCallback) {
	SysWnd* mainWnd        = nullptr;
	auto    mainCallbackEv = [mainCallback, &mainWnd](MainEvent ev) {
        switch (ev) {
        case MainEventInit:
            mainWnd = SysWnd::CreateWithDoc();
            mainWnd->Show();
            mainCallback(mainWnd);
            break;
        case MainEventShutdown:
            delete mainWnd;
            mainWnd = nullptr;
            break;
        }
    };
	RunAppLowLevel(mainCallbackEv);
}

XO_API Style** DefaultTagStyles() {
	return StaticDefaultTagStyles;
}

XO_API void ParseFail(const char* msg, ...) {
	char    buff[2048];
	va_list va;
	va_start(va, msg);
	uint32_t r = vsnprintf(buff, arraysize(buff), msg, va);
	va_end(va);
	buff[arraysize(buff) - 1] = 0;
	if (r < arraysize(buff))
		XO_TRACE_WRITE(buff);
}

XO_API void Trace(const char* msg, ...) {
	char    buff[2048];
	va_list va;
	va_start(va, msg);
	uint32_t r = vsnprintf(buff, arraysize(buff), msg, va);
	va_end(va);
	buff[arraysize(buff) - 1] = 0;
	if (r < arraysize(buff))
		XO_TRACE_WRITE(buff);
}

XO_API void TimeTrace(const char* msg, ...) {
	const int timeChars = 16;
	char      buff[2048];
	sprintf(buff, "%-15.3f  ", TimeAccurateSeconds() * 1000);
	va_list va;
	va_start(va, msg);
	uint32_t r = vsnprintf(buff + timeChars, arraysize(buff) - timeChars, msg, va);
	va_end(va);
	buff[arraysize(buff) - 1] = 0;
	if (r < arraysize(buff))
		XO_TRACE_WRITE(buff);
}
}
