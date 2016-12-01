#pragma once

// This is the bottom-level include file that many other xo headers include. It should
// strive to remain quite small.

#include "xoPlatform.h"
#include "xoTags.h"

class xoBox;
class xoBox16;
class xoBoxF;
class xoDomEl;
class xoDomCanvas;
class xoDomNode;
class xoDomText;
class xoDoc;
class xoDocUI;
class xoCanvas2D;
class xoEvent;
class xoOriginalEvent;
class xoImage;
class xoImageStore;
class xoLayout;
class xoLayout2;
class xoLayout3;
class xoLayoutResult;
class xoPool;
class xoDocGroup;
class xoRenderDoc;
class xoRenderer;
class xoRenderDomEl;
class xoRenderDomNode;
class xoRenderDomText;
struct xoRenderCharEl;
class xoRenderBase;
class xoRenderGL;
class xoRenderDX;
class xoString;
class xoStringTable;
class xoStyle;
class xoSysWnd;
class xoFont;
class xoFontStore;
class xoGlyphCache;
class xoTextureAtlas;
#ifndef XO_MAT4F_DEFINED
class xoMat4f;
#endif

typedef int32 xoPos;								// fixed-point position
static xoPos xoPosNULL = INT32MAX;
static const u32 xoPosShift = 8;					// 24:8 fixed point coordinates used during layout
static const u32 xoPosMask = (1 << xoPosShift) - 1;	// 255

#define XO_PI 3.14159265358979323846264338327950288

// An ID that is internal to xo - i.e. it is not controllable by external code.
// This ID is an integer that you can use to reference a DOM element. These IDs are recycled.
typedef int32 xoInternalID;
static const xoInternalID xoInternalIDNull = 0;		// Zero is always an invalid DOM element ID
static const xoInternalID xoInternalIDRoot = 1;		// The root of the DOM tree always has ID = 1

typedef int32 xoFontID;
static const xoFontID xoFontIDNull = 0;				// Zero is always an invalid Font ID

// Handle to a texture that is (maybe) resident in the graphics driver.
// xo supports the concept of the graphics device being "lost", so just because you have
// a non-zero xoTextureID, does not mean that the ID is valid. Prior to drawing the scene,
// the texture loading functions must check whether the ID is still valid or not.
// Note that there is a lot of unsigned arithmetic used by the texture management facilities,
// so this data type must remain unsigned.
typedef uint32 xoTextureID;
static const xoTextureID xoTextureIDNull = 0;		// Zero is always an invalid Texture ID

// Maximum number of texture units that we will try to use
static const u32 xoMaxTextureUnits = 8;

inline int32	xoIntToPos(int real)			{ return real << xoPosShift; }
inline int32	xoRealx256ToPos(int32 real)		{ return int32(real * ((1 << xoPosShift) / 256)); }   // Since xoPosShift = 256, xoRealx256ToPos simplifies out to identity
inline int32	xoRealToPos(float real)			{ return int32(real * (1 << xoPosShift)); }
inline int32	xoDoubleToPos(double real)		{ return int32(real * (1 << xoPosShift)); }
inline float	xoPosToReal(int32 pos)			{ return pos * (1.0f / (1 << xoPosShift)); }
inline double	xoPosToDouble(int32 pos)		{ return pos * (1.0 / (1 << xoPosShift)); }
inline int32	xoPosRound(int32 pos)			{ return pos + (1 << (xoPosShift-1)) & ~xoPosMask; }
inline int32	xoPosRoundDown(int32 pos)		{ return pos & ~xoPosMask; }
inline int32	xoPosRoundUp(int32 pos)			{ return pos + ((1 << xoPosShift) - 1) & ~xoPosMask; }
inline float	xoRound(float real)				{ return floor(real + 0.5f); }

template<typename T>
int xoSign(T real)							{ return (real == 0) ? 0 : (real < 0 ? -1 : 1); }

template<typename T>
T xoLerp(T pos, T a, T b)					{ return a * (1 - pos) + b * pos; }

enum xoCloneFlags
{
	xoCloneFlagEvents = 1,		// Include events in clone
};

static const int XO_MAX_TOUCHES = 10;

enum xoMainEvent
{
	xoMainEventInit = 1,
	xoMainEventShutdown,
};

enum xoRenderResult
{
	// SYNC-JAVA
	xoRenderResultNeedMore,
	xoRenderResultIdle
};

enum xoEndRenderFlags
{
	xoEndRenderNoSwap = 1,		// Do not call SwapBuffers() or SwapChain->Present(). This frame is going to be discarded.
};

enum xoCursors
{
	xoCursorArrow,
	xoCursorHand,
	xoCursorText,
	xoCursorWait,
};

//struct xoVec2
//{
//	float x,y;
//};
inline xoVec2f XOVEC2(float x, float y) { return xoVec2f(x,y); }

//struct xoVec3
//{
//	float x,y,z;
//};
inline xoVec3f XOVEC3(float x, float y, float z) { return xoVec3f(x,y,z); }

//struct xoVec4
//{
//	float x,y,z,w;
//};
inline xoVec4f XOVEC4(float x, float y, float z, float w) { return xoVec4f(x,y,z,w); }

class XOAPI xoPoint
{
public:
	xoPos	X, Y;

	xoPoint() : X(0), Y(0) {}
	xoPoint(xoPos x, xoPos y) : X(x), Y(y) {}

	void		SetInt(int32 x, int32 y)				{ X = xoRealToPos((float) x); Y = xoRealToPos((float) y); }
	bool		operator==(const xoPoint& p) const	{ return X == p.X && Y == p.Y; }
	bool		operator!=(const xoPoint& p) const	{ return !(*this == p); }
	xoPoint		operator+(const xoPoint& p) const		{ return xoPoint(X + p.X, Y + p.Y); }
	xoPoint		operator-(const xoPoint& p) const		{ return xoPoint(X - p.X, Y - p.Y); }
	xoPoint&	operator+=(const xoPoint& p)			{ X += p.X; Y += p.Y; return *this; }
	xoPoint&	operator-=(const xoPoint& p)			{ X -= p.X; Y -= p.Y; return *this; }
};

/*
Why does this class have a copy constructor and assignment operator?
Without those, we get data alignment exceptions (signal 7) when running on my Galaxy S3.
I tried explicitly raising the alignment of xoBox to 8 and 16 bytes, but that did not help.
Unfortunately I have not yet had the energy to open up the assembly and see what the compiler is doing wrong.
This is documented inside xo/docs/android.md

There are some magic values in here:
xoBox is used to represent the content-box of a node.
One thing that comes up frequently is that a box has a well defined Left, but no Right. Or vice versa, or any combination thereof.
So, xoPosNULL is treated like a NaN. If either Left or Right is xoPosNULL, then WidthOrNull is xoPosNULL.
Likewise for HeightOrNull.
*/
class XOAPI xoBox
{
public:
	xoPos	Left;		// X1
	xoPos	Top;		// Y1
	xoPos	Right;		// X2
	xoPos	Bottom;		// Y2

	xoBox() : Left(0), Right(0), Top(0), Bottom(0) {}
	xoBox(const xoBox& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	xoBox(xoPos left, xoPos top, xoPos right, xoPos bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}

#ifdef _WIN32
	xoBox(RECT r) : Left(r.left), Right(r.right), Top(r.top), Bottom(r.bottom) {}
	operator RECT() const { RECT r = {Left, Top, Right, Bottom}; return r; }
#endif

	static xoBox Inverted() { return xoBox(INT32MAX, INT32MAX, INT32MIN, INT32MIN); }

	void	SetInt(int32 left, int32 top, int32 right, int32 bottom);
	void	ExpandToFit(const xoBox& expando);
	void	ClampTo(const xoBox& clamp);
	xoBox	ShrunkBy(const xoBox& marginBox);
	xoBox	PiecewiseSum(const xoBox& box);
	xoBoxF	ToRealBox() const;

	void	SetInverted()							{ *this = Inverted(); }
	bool	IsAreaPositive() const					{ return Right > Left && Bottom > Top; }
	xoPos	Width() const							{ return Right - Left; }
	xoPos	Height() const							{ return Bottom - Top; }
	xoPos	WidthOrNull() const						{ return (Left == xoPosNULL || Right == xoPosNULL) ? xoPosNULL : Right - Left; }
	xoPos	HeightOrNull() const					{ return (Top == xoPosNULL || Bottom == xoPosNULL) ? xoPosNULL : Bottom - Top; }
	xoPoint	TopLeft() const							{ return xoPoint(Left, Top); }
	void	Offset(int32 x, int32 y)				{ Left += x; Right += x; Top += y; Bottom += y; }
	void	Offset(xoPoint p)						{ Offset(p.X, p.Y); }
	xoBox	OffsetBy(int32 x, int32 y)				{ return xoBox(Left + x, Top + y, Right + x, Bottom + y); }
	xoBox	OffsetBy(xoPoint p)						{ return xoBox(Left + p.X, Top + p.Y, Right + p.X, Bottom + p.Y); }
	bool	IsInsideMe(xoPoint p) const				{ return p.X >= Left && p.Y >= Top && p.X < Right && p.Y < Bottom; }
	bool	IsAreaZero() const						{ return Width() == 0 || Height() == 0; }

	bool operator==(const xoBox& b) { return Left == b.Left && Right == b.Right && Top == b.Top && Bottom == b.Bottom; }
	bool operator!=(const xoBox& b) { return !(*this == b); }

	// $XO_GCC_ALIGN_BUG
	xoBox&	operator=(const xoBox& b) { Left = b.Left; Right = b.Right; Top = b.Top; Bottom = b.Bottom; return *this; }
};

class XOAPI xoBox16
{
public:
	uint16	Left;
	uint16	Top;
	uint16	Right;
	uint16	Bottom;

	xoBox16() : Left(0), Right(0), Top(0), Bottom(0) {}
	xoBox16(const xoBox& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	xoBox16(const xoBox16& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	xoBox16(xoPos left, xoPos top, xoPos right, xoPos bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}

	xoBoxF	ToRealBox() const;
};

class XOAPI xoBoxF
{
public:
	float	Left, Right, Top, Bottom;

	xoBoxF() : Left(0), Right(0), Top(0), Bottom(0) {}
	xoBoxF(float left, float top, float right, float bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}
};

struct XOAPI xoRGBA
{
	union {
		struct {
#if ENDIANLITTLE
			uint8 a, b, g, r;
#else
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
			uint8 a: 8;
#endif
		};
		uint32 u;
	};
	static xoRGBA RGBA(uint8 r, uint8 g, uint8 b, uint8 a) { xoRGBA c; c.r = r; c.g = g; c.b = b; c.a = a; return c; }
};

// This is non-premultipled alpha
struct XOAPI xoColor
{
	union {
		struct {
#if ENDIANLITTLE
			uint8 b, g, r, a;
#else
			uint8 a: 8;
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
#endif
		};
		uint32 u;
	};

	void	Set(uint8 _r, uint8 _g, uint8 _b, uint8 _a) { r = _r; g = _g; b = _b; a = _a; }
	uint32	GetRGBA() const { xoRGBA x; x.r = r; x.g = g; x.b = b; x.a = a; return x.u; }
	xoVec4f	GetVec4sRGB() const;
	xoVec4f	GetVec4Linear() const;

	bool	operator==(const xoColor& x) const { return u == x.u; }
	bool	operator!=(const xoColor& x) const { return u != x.u; }

	static bool		Parse(const char* s, intp len, xoColor& v);
	static xoColor	RGBA(uint8 _r, uint8 _g, uint8 _b, uint8 _a)		{ xoColor c; c.Set(_r,_g,_b,_a); return c; }
	static xoColor	Make(uint32 _u)										{ xoColor c; c.u = _u; return c; }
	static xoColor	Black()												{ return xoColor::RGBA(0,0,0,255); }
	static xoColor	White()												{ return xoColor::RGBA(255,255,255,255); }
	static xoColor	Transparent()										{ return xoColor::RGBA(0,0,0,0); }
};

XOAPI float	xoSRGB2Linear(uint8 srgb);
XOAPI uint8	xoLinear2SRGB(float linear);

struct xoStyleClassID
{
	uint32		StyleClassID;

	xoStyleClassID()			: StyleClassID(0)	{}
	explicit	xoStyleClassID(uint32 id)	: StyleClassID(id)	{}

	operator	uint32() const { return StyleClassID; }
};

struct xoJob
{
	void*	JobData;
	void (*JobFunc)(void* jobdata);
};

struct XOAPI xoRenderStats
{
	uint32	Clone_NumEls;		// Number of DOM elements cloned

	void Reset();
};

#define XO_MAKE_TEX_FORMAT(premultiplied, nchannels)  ( ((premultiplied) << 3) | (nchannels) )

enum xoTexFormat
{
	xoTexFormatInvalid		= XO_MAKE_TEX_FORMAT(0,0),
	xoTexFormatRGBA8		= XO_MAKE_TEX_FORMAT(1,4),
	xoTexFormatGrey8		= XO_MAKE_TEX_FORMAT(0,1),
};

#undef XO_MAKE_TEX_FORMAT

inline int xoTexFormatChannelCount(xoTexFormat f)			{ return f & 7; }
inline int xoTexFormatBytesPerChannel(xoTexFormat f)		{ return 1; }
inline int xoTexFormatBytesPerPixel(xoTexFormat f)		{ return xoTexFormatBytesPerChannel(f) * xoTexFormatChannelCount(f); }

enum xoTexFilter
{
	xoTexFilterNearest,
	xoTexFilterLinear,
};

/* Base of all textures
This structure must remain zero-initializable
Once a texture has been uploaded, you may not change width, height, channel count, filter.
*/
class XOAPI xoTexture
{
public:
	uint32		TexWidth		= 0;
	uint32		TexHeight		= 0;
	xoBox		TexInvalidRect	= xoBox::Inverted();	// Invalid rectangle, in integer texel coordinates.
	xoTextureID	TexID			= xoInternalIDNull;		// ID of texture in renderer.
	xoTexFormat	TexFormat		= xoTexFormatInvalid;
	void*		TexData			= nullptr;
	int			TexStride		= 0;
	xoTexFilter	TexFilterMin	= xoTexFilterLinear;
	xoTexFilter	TexFilterMax	= xoTexFilterLinear;

	void	TexInvalidateWholeSurface()	{ TexInvalidRect = xoBox(0, 0, TexWidth, TexHeight); }
	void	TexClearInvalidRect()		{ TexInvalidRect.SetInverted(); }
	void*	TexDataAt(int x, int y)	{ return ((char*) TexData) + y * TexStride + x * xoTexFormatBytesPerPixel(TexFormat); }
	void*	TexDataAtLine(int y)		{ return ((char*) TexData) + y * TexStride; }
	size_t	TexBytesPerPixel() const	{ return xoTexFormatBytesPerPixel(TexFormat); }
	void	FlipVertical();
};

// Base of GL and DX shader programs
class xoProgBase
{
public:
};

enum xoShaders
{
	xoShaderInvalid,
	xoShaderFill,
	xoShaderFillTex,
	xoShaderRect,
	xoShaderRect2,
	xoShaderRect3,
	xoShaderTextRGB,
	xoShaderTextWhole,
	xoShaderArc,
	xoShaderQuadraticSpline,
	xoShaderUber,
	// We may someday want to have slots down here available for application-defined custom shaders
};

// A single instance of this is accessible via xoGlobal()
struct xoGlobalStruct
{
	int							TargetFPS;
	int							NumWorkerThreads;		// Read-only. Set during xoInitialize().
	int							MaxSubpixelGlyphSize;	// Maximum font size where we will use sub-pixel glyph textures
	bool						PreferOpenGL;			// Prefer OpenGL over DirectX. If this is true, then on Windows OpenGL will be tried first.
	bool						EnableVSync;			// This is only respected during device initialization, so you must set it at application start. It raises latency noticeably. This has no effect on DirectX windowed rendering.
	bool						EnableSubpixelText;		// Enable sub-pixel text rendering. Assumes pixels are the standard RGB layout. Enabled by default on Windows desktop only.
	bool						EnableSRGBFramebuffer;	// Enable sRGB framebuffer (implies linear blending)
	bool						EnableKerning;			// Enable kerning on text
	bool						RoundLineHeights;		// Round text line heights to integer amounts, so that text line separation is not subject to sub-pixel positioning differences.
	bool						UseRect3;				// Use 3rd attempt at box shader
	bool						SnapBoxes;				// Round certain boxes up to integer pixels.
														// From the perspective of having the exact same layout on multiple devices, it seems desirable to operate
														// in subpixel coordinates always. However, this ends up producing ugly visuals, for example when
														// you have a box with a single pixel border, and it is not aligned to a pixel boundary, then you get
														// the border smudged across two pixels.
	bool						SnapSubpixelHorzText;	// When rendering subpixel text, snap glyphs to whole pixels, instead of sub-pixel horizontal positioning.
														// This not only determines layout behaviour, but also how our subpixel glyphs are rasterized.
	//bool						EmulateGammaBlending;	// Only applicable when EnableSRGBFramebuffer = true, this tries to emulate gamma-space blending. You would turn this on to get consistent blending on all devices. FAILED EXPERIMENT - BAD IDEA.
	float						SubPixelTextGamma;		// Tweak freetype's gamma when doing sub-pixel text rendering. Should be no need to use anything other than 1.0
	float						WholePixelTextGamma;	// Tweak freetype's gamma when doing whole-pixel text rendering. Should be no need to use anything other than 1.0
	float						EpToPixel;				// Eye Pixel to Pixel.
	xoTextureID					MaxTextureID;			// Used to test texture ID wrap-around. Were it not for testing, this could be 2^32 - 1
	xoColor						ClearColor;				// glClearColor
	xoString					CacheDir;				// Root directory where we store font caches, etc. Overridable with xoInitParams

	// Debugging flags. Enabling these should make debugging easier.
	// Some of them may turn out to have a small enough performance hit that you can
	// leave them turned on always.
	// NOPE.. it's just too confusing to have this optional. It's always on.
	//bool						DebugZeroClonedChildList;	// During a document clone, zero out ChildByInternalID before populating. This will ensure that gaps are NULL instead of random memory.

	pvect<xoDocGroup*>			Docs;				// Only Main thread is allowed to touch this.
	TAbcQueue<xoDocGroup*>		DocAddQueue;		// Documents requesting addition
	TAbcQueue<xoDocGroup*>		DocRemoveQueue;		// Documents requesting removal
	TAbcQueue<xoOriginalEvent>	UIEventQueue;		// Global event queue, consumed by the one-and-only UI thread
	TAbcQueue<xoJob>			JobQueue;			// Global job queue, consumed by the worker thread pool
	xoFontStore*				FontStore;			// All fonts known to the system.
	xoGlyphCache*				GlyphCache;			// This might have to move into a less global domain.
};

// Optional initialization parameters
struct xoInitParams
{
	float		EpToPixel = 0;		// Override Eye Pixels to Device Pixels (ends up in xoGlobalStruct.EpToPixel)
	xoString	CacheDir;			// Override cache directory used for font caches etc.
};

typedef std::function<void(xoMainEvent ev)>		xoMainCallbackLowLevel;
typedef std::function<void(xoSysWnd* wnd)>		xoMainCallback;

XOAPI xoGlobalStruct*	xoGlobal();
XOAPI void				xoInitialize(const xoInitParams* init = nullptr);
XOAPI void				xoShutdown();
XOAPI void				xoRunAppLowLevel(xoMainCallbackLowLevel mainCallback);
XOAPI void				xoRunApp(xoMainCallback mainCallback);
XOAPI void				xoAddOrRemoveDocsFromGlobalList();
XOAPI void				xoParseFail(const char* msg, ...);
XOAPI void				XOTRACE(const char* msg, ...);
XOAPI void				XOTIME(const char* msg, ...);
#if XO_PLATFORM_WIN_DESKTOP
XOAPI void				xoRunWin32MessageLoop();
#elif XO_PLATFORM_LINUX_DESKTOP
XOAPI void				xoRunXMessageLoop();
#endif

// Various tracing options. Uncomment these to enable tracing of that class of events.
//#define XOTRACE_RENDER_ENABLE
//#define XOTRACE_LAYOUT_VERBOSE_ENABLE
//#define XOTRACE_EVENTS_ENABLE
//#define XOTRACE_LATENCY_ENABLE
//#define XOTRACE_FONTS_ENABLE
//#define XOTRACE_OS_MSG_QUEUE_ENABLE

#define XOTRACE_WARNING_ENABLE
#define XOTRACE_LAYOUT_WARNINGS_ENABLE

#ifdef XOTRACE_RENDER_ENABLE
#define XOTRACE_RENDER(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_RENDER(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_LAYOUT_WARNINGS_ENABLE
#define XOTRACE_LAYOUT_WARNING(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LAYOUT_WARNING(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_LAYOUT_VERBOSE_ENABLE
#define XOTRACE_LAYOUT_VERBOSE(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LAYOUT_VERBOSE(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_EVENTS_ENABLE
#define XOTRACE_EVENTS(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_EVENTS(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_LATENCY_ENABLE
#define XOTRACE_LATENCY(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LATENCY(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_FONTS_ENABLE
#define XOTRACE_FONTS(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_FONTS(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_OS_MSG_QUEUE_ENABLE
#define XOTRACE_OS_MSG_QUEUE(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_OS_MSG_QUEUE(msg, ...) ((void)0)
#endif

#ifdef XOTRACE_WARNING_ENABLE
#define XOTRACE_WARNING(msg, ...) XOTIME(msg, ##__VA_ARGS__)
#else
#define XOTRACE_WARNING(msg, ...) ((void)0)
#endif
