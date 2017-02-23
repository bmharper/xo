#pragma once

// This is the bottom-level include file that many other xo headers include. It should
// strive to remain quite small.

#include "Tags.h"

namespace xo {

class Box;
class Box16;
class BoxF;
class DomEl;
class DomCanvas;
class DomNode;
class DomText;
class Doc;
class DocUI;
class Canvas2D;
class Event;
class OriginalEvent;
class Image;
class ImageStore;
class Layout;
class LayoutResult;
class Pool;
class DocGroup;
class RenderDoc;
class Renderer;
class RenderDomEl;
class RenderDomNode;
class RenderDomText;
struct RenderCharEl;
class RenderBase;
class RenderGL;
class RenderDX;
class String;
class StringTable;
class Style;
class SysWnd;
class Font;
class FontStore;
class GlyphCache;
class TextureAtlas;
#ifndef XO_MAT4F_DEFINED
class Mat4f;
#endif

typedef int32_t       Pos; // fixed-point position
static Pos            PosNULL  = INT32_MAX;
static const uint32_t PosShift = 8;                   // 24:8 fixed point coordinates used during layout
static const uint32_t PosMask  = (1 << PosShift) - 1; // 255

#define XO_PI 3.14159265358979323846264338327950288

// An ID that is internal to xo - i.e. it is not controllable by external code.
// This ID is an integer that you can use to reference a DOM element. These IDs are recycled.
typedef uint32_t        InternalID;
static const InternalID InternalIDNull = 0; // Zero is always an invalid DOM element ID. Must be zero - code assumes it can zero-initialize this.
static const InternalID InternalIDRoot = 1; // The root of the DOM tree always has ID = 1

typedef int32_t     FontID;
static const FontID FontIDNull = 0; // Zero is always an invalid Font ID

// Handle to a texture that is (maybe) resident in the graphics driver.
// xo supports the concept of the graphics device being "lost", so just because you have
// a non-zero TextureID, does not mean that the ID is valid. Prior to drawing the scene,
// the texture loading functions must check whether the ID is still valid or not.
// Note that there is a lot of unsigned arithmetic used by the texture management facilities,
// so this data type must remain unsigned.
typedef uint32_t       TextureID;
static const TextureID TextureIDNull = 0; // Zero is always an invalid Texture ID

// Maximum number of texture units that we will try to use
static const uint32_t MaxTextureUnits = 8;

inline int32_t IntToPos(int real) { return real << PosShift; }
inline int32_t Realx256ToPos(int32_t real) { return int32_t(real * ((1 << PosShift) / 256)); } // Since PosShift = 256, Realx256ToPos simplifies out to identity
inline int32_t RealToPos(float real) { return int32_t(real * (1 << PosShift)); }
inline int32_t DoubleToPos(double real) { return int32_t(real * (1 << PosShift)); }
inline float   PosToReal(int32_t pos) { return pos * (1.0f / (1 << PosShift)); }
inline double  PosToDouble(int32_t pos) { return pos * (1.0 / (1 << PosShift)); }
inline int32_t PosRound(int32_t pos) { return pos + (1 << (PosShift - 1)) & ~PosMask; }
inline int32_t PosRoundDown(int32_t pos) { return pos & ~PosMask; }
inline int32_t PosRoundUp(int32_t pos) { return pos + ((1 << PosShift) - 1) & ~PosMask; }
inline int32_t PosMul(int32_t a, int32_t b) { return (a * b) >> PosShift; }
inline float   Round(float real) { return floor(real + 0.5f); }

// This is Jim Blinn's ubyte*ubyte multiplier, which is 100% accurate
inline uint8_t MulUBGood(uint8_t a, uint8_t b) {
	uint32_t i = (uint32_t) a * (uint32_t) b + 128;
	return (uint8_t)((i + (i >> 8)) >> 8);
}

// This is a cheap ubyte*ubyte multiplier, which maintains 0*1 = 0 and 1*1 = 1
inline uint8_t MulUBCheap(uint8_t a, uint8_t b) {
	uint32_t i = (uint32_t) a * ((uint32_t) b + 1);
	return (uint8_t)(i >> 8);
}

template <typename T>
int Sign(T real) {
	return (real == 0) ? 0 : (real < 0 ? -1 : 1);
}

template <typename T>
T Lerp(T pos, T a, T b) {
	return a * (1 - pos) + b * pos;
}

enum CloneFlags {
	CloneFlagEvents = 1, // Include events in clone
};

static const int XO_MAX_TOUCHES = 10;

enum MainEvent {
	MainEventInit = 1,
	MainEventShutdown,
};

enum RenderResult {
	// SYNC-JAVA
	RenderResultNeedMore,
	RenderResultDone
};

enum EndRenderFlags {
	EndRenderNoSwap = 1, // Do not call SwapBuffers() or SwapChain->Present(). This frame is going to be discarded.
};

enum Cursors {
	CursorArrow,
	CursorHand,
	CursorText,
	CursorWait,
};

inline Vec2f VEC2(float x, float y) { return Vec2f(x, y); }
inline Vec3f VEC3(float x, float y, float z) { return Vec3f(x, y, z); }
inline Vec4f VEC4(float x, float y, float z, float w) { return Vec4f(x, y, z, w); }
class XO_API Point {
public:
	Pos X, Y;

	Point() : X(0), Y(0) {}
	Point(Pos x, Pos y) : X(x), Y(y) {}
	void SetInt(int32_t x, int32_t y) {
		X = RealToPos((float) x);
		Y = RealToPos((float) y);
	}
	bool   operator==(const Point& p) const { return X == p.X && Y == p.Y; }
	bool   operator!=(const Point& p) const { return !(*this == p); }
	Point  operator+(const Point& p) const { return Point(X + p.X, Y + p.Y); }
	Point  operator-(const Point& p) const { return Point(X - p.X, Y - p.Y); }
	Point& operator+=(const Point& p) {
		X += p.X;
		Y += p.Y;
		return *this;
	}
	Point& operator-=(const Point& p) {
		X -= p.X;
		Y -= p.Y;
		return *this;
	}
	Vec2f ToReal() const { return Vec2f(PosToReal(X), PosToReal(Y)); }
};

/*
Why does this class have a copy constructor and assignment operator?
Without those, we get data alignment exceptions (signal 7) when running on my Galaxy S3.
I tried explicitly raising the alignment of Box to 8 and 16 bytes, but that did not help.
Unfortunately I have not yet had the energy to open up the assembly and see what the compiler is doing wrong.
This is documented inside xo/docs/android.md

There are some magic values in here:
Box is used to represent the content-box of a node.
One thing that comes up frequently is that a box has a well defined Left, but no Right. Or vice versa, or any combination thereof.
So, PosNULL is treated like a NaN. If either Left or Right is PosNULL, then WidthOrNull is PosNULL.
Likewise for HeightOrNull.
*/
class XO_API Box {
public:
	Pos Left;   // X1
	Pos Top;    // Y1
	Pos Right;  // X2
	Pos Bottom; // Y2

	Box() : Left(0), Right(0), Top(0), Bottom(0) {}
	Box(const Box& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	Box(Pos left, Pos top, Pos right, Pos bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}
#ifdef _WIN32
	Box(RECT r) : Left(r.left), Right(r.right), Top(r.top), Bottom(r.bottom) {
	}
	operator RECT() const {
		RECT r = {Left, Top, Right, Bottom};
		return r;
	}
#endif

	static Box Inverted() {
		return Box(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);
	}

	void SetInt(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void ExpandToFit(const Box& expando);
	void ClampTo(const Box& clamp);
	Box  ShrunkBy(const Box& marginBox);
	Box  PiecewiseSum(const Box& box);
	BoxF ToRealBox() const;

	void  SetInverted() { *this = Inverted(); }
	bool  IsAreaPositive() const { return Right > Left && Bottom > Top; }
	Pos   Width() const { return Right - Left; }
	Pos   Height() const { return Bottom - Top; }
	Pos   WidthOrNull() const { return (Left == PosNULL || Right == PosNULL) ? PosNULL : Right - Left; }
	Pos   HeightOrNull() const { return (Top == PosNULL || Bottom == PosNULL) ? PosNULL : Bottom - Top; }
	Point TopLeft() const { return Point(Left, Top); }
	void  Offset(int32_t x, int32_t y) {
        Left += x;
        Right += x;
        Top += y;
        Bottom += y;
	}
	void Offset(Point p) { Offset(p.X, p.Y); }
	Box  OffsetBy(int32_t x, int32_t y) { return Box(Left + x, Top + y, Right + x, Bottom + y); }
	Box  OffsetBy(Point p) { return Box(Left + p.X, Top + p.Y, Right + p.X, Bottom + p.Y); }
	bool IsInsideMe(Point p) const { return p.X >= Left && p.Y >= Top && p.X < Right && p.Y < Bottom; }
	bool IsAreaZero() const { return Width() == 0 || Height() == 0; }
	bool operator==(const Box& b) { return Left == b.Left && Right == b.Right && Top == b.Top && Bottom == b.Bottom; }
	bool operator!=(const Box& b) { return !(*this == b); }
	// $XO_GCC_ALIGN_BUG
	Box& operator=(const Box& b) {
		Left   = b.Left;
		Right  = b.Right;
		Top    = b.Top;
		Bottom = b.Bottom;
		return *this;
	}
};

// The values in here have either 8 bits of sub-pixel precision, like Pos values,
// or they have 2-bit sub-pixel precision. The 2-bit subpixel precision was added for
// the sake of the border-radius values, where a limit of 256 is too low for drawing
// a circle that fills the screen.
class XO_API Box16 {
public:
	uint16_t Left;
	uint16_t Top;
	uint16_t Right;
	uint16_t Bottom;

	Box16() : Left(0), Right(0), Top(0), Bottom(0) {}
	Box16(const Box& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	Box16(const Box16& b) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	Box16(Pos left, Pos top, Pos right, Pos bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}
	void Set2BitPrecision(const Box& b);
	BoxF ToRealBox() const;
	BoxF ToRealBox2BitPrecision() const;
};

class XO_API BoxF {
public:
	float Left;
	float Top;
	float Right;
	float Bottom;

	BoxF() : Left(0), Right(0), Top(0), Bottom(0) {}
	BoxF(float left, float top, float right, float bottom) : Left(left), Right(right), Top(top), Bottom(bottom) {}
	bool operator==(const BoxF& b) { return Left == b.Left && Right == b.Right && Top == b.Top && Bottom == b.Bottom; }
	bool operator!=(const BoxF& b) { return !(*this == b); }
};

struct XO_API RGBA {
	union {
		struct
		{
#if ENDIANLITTLE
			uint8_t a, b, g, r;
#else
			uint8_t r : 8;
			uint8_t g : 8;
			uint8_t b : 8;
			uint8_t a : 8;
#endif
		};
		uint32_t u;
	};
	static RGBA Make(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		RGBA c;
		c.r = r;
		c.g = g;
		c.b = b;
		c.a = a;
		return c;
	}
};

// This is non-premultipled alpha
struct XO_API Color {
	union {
		struct
		{
#if ENDIANLITTLE
			uint8_t b, g, r, a;
#else
			uint8_t a : 8;
			uint8_t r : 8;
			uint8_t g : 8;
			uint8_t b : 8;
#endif
		};
		uint32_t u;
	};

	Color() {}
	Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
	void Set(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
	uint32_t GetRGBA() const {
		xo::RGBA x;
		x.r = r;
		x.g = g;
		x.b = b;
		x.a = a;
		return x.u;
	}
	Vec4f GetVec4sRGB() const;
	Vec4f GetVec4Linear() const;
	Color Premultiply() const {
		return Color::RGBA(MulUBGood(r, a), MulUBGood(g, a), MulUBGood(b, a), a);
	}

	bool         operator==(const Color& x) const { return u == x.u; }
	bool         operator!=(const Color& x) const { return u != x.u; }
	static bool  Parse(const char* s, size_t len, Color& v);
	static Color RGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
		Color c;
		c.Set(_r, _g, _b, _a);
		return c;
	}
	static Color Make(uint32_t _u) {
		Color c;
		c.u = _u;
		return c;
	}
	static Color Black() { return Color::RGBA(0, 0, 0, 255); }
	static Color White() { return Color::RGBA(255, 255, 255, 255); }
	static Color Transparent() { return Color::RGBA(0, 0, 0, 0); }
};

XO_API float SRGB2Linear(uint8_t srgb);
XO_API uint8_t Linear2SRGB(float linear);

struct StyleClassID {
	uint32_t ID;

	StyleClassID() : ID(0) {}
	explicit StyleClassID(uint32_t id) : ID(id) {}
	operator uint32_t() const { return ID; }
};

struct Job {
	void* JobData;
	void (*JobFunc)(void* jobdata);
};

struct XO_API RenderStats {
	uint32_t Clone_NumEls; // Number of DOM elements cloned

	void Reset();
};

#define XO_MAKE_TEX_FORMAT(premultiplied, nchannels) (((premultiplied) << 3) | (nchannels))

enum TexFormat {
	TexFormatInvalid = XO_MAKE_TEX_FORMAT(0, 0),
	TexFormatRGBA8   = XO_MAKE_TEX_FORMAT(1, 4),
	TexFormatGrey8   = XO_MAKE_TEX_FORMAT(0, 1),
};

#undef XO_MAKE_TEX_FORMAT

inline int TexFormatChannelCount(TexFormat f) {
	return f & 7;
}
inline int TexFormatBytesPerChannel(TexFormat f) { return 1; }
inline int TexFormatBytesPerPixel(TexFormat f) { return TexFormatBytesPerChannel(f) * TexFormatChannelCount(f); }
enum TexFilter {
	TexFilterNearest,
	TexFilterLinear,
};

/* Base of all textures
This structure must remain zero-initializable
Once a texture has been uploaded, you may not change width, height, channel count, filter.
*/
class XO_API Texture {
public:
	uint32_t  TexWidth       = 0;
	uint32_t  TexHeight      = 0;
	Box       TexInvalidRect = Box::Inverted(); // Invalid rectangle, in integer texel coordinates.
	TextureID TexID          = InternalIDNull;  // ID of texture in renderer.
	TexFormat TexFormat      = TexFormatInvalid;
	void*     TexData        = nullptr;
	int       TexStride      = 0;
	TexFilter TexFilterMin   = TexFilterLinear;
	TexFilter TexFilterMax   = TexFilterLinear;

	void   TexInvalidateWholeSurface() { TexInvalidRect = Box(0, 0, TexWidth, TexHeight); }
	void   TexClearInvalidRect() { TexInvalidRect.SetInverted(); }
	void*  TexDataAt(int x, int y) { return ((char*) TexData) + y * TexStride + x * TexFormatBytesPerPixel(TexFormat); }
	void*  TexDataAtLine(int y) { return ((char*) TexData) + y * TexStride; }
	size_t TexBytesPerPixel() const { return TexFormatBytesPerPixel(TexFormat); }
	void   FlipVertical();
};

// Base of GL and DX shader programs
class ProgBase {
public:
};

enum Shaders {
	ShaderInvalid,
	ShaderFill,
	ShaderFillTex,
	ShaderRect,
	ShaderRect2,
	ShaderRect3,
	ShaderTextRGB,
	ShaderTextWhole,
	ShaderArc,
	ShaderQuadraticSpline,
	ShaderUber,
	// We may someday want to have slots down here available for application-defined custom shaders
};

// A single instance of this is accessible via Global()
struct GlobalStruct {
	int  TargetFPS;
	int  NumWorkerThreads;      // Read-only. Set during Initialize().
	int  MaxSubpixelGlyphSize;  // Maximum font size where we will use sub-pixel glyph textures
	bool PreferOpenGL;          // Prefer OpenGL over DirectX. If this is true, then on Windows OpenGL will be tried first.
	bool EnableVSync;           // This is only respected during device initialization, so you must set it at application start. It raises latency noticeably. This has no effect on DirectX windowed rendering.
	bool EnableSubpixelText;    // Enable sub-pixel text rendering. Assumes pixels are the standard RGB layout. Enabled by default on Windows desktop only.
	bool EnableSRGBFramebuffer; // Enable sRGB framebuffer (implies linear blending)
	bool EnableKerning;         // Enable kerning on text
	bool RoundLineHeights;      // Round text line heights to integer amounts, so that text line separation is not subject to sub-pixel positioning differences.
	bool SnapBoxes;             // Round certain boxes up to integer pixels.
	                            // From the perspective of having the exact same layout on multiple devices, it seems desirable to operate
	                            // in subpixel coordinates always. However, this ends up producing ugly visuals, for example when
	                            // you have a box with a single pixel border, and it is not aligned to a pixel boundary, then you get
	                            // the border smudged across two pixels.
	bool SnapHorzText;          // Snap glyphs to whole pixels, instead of sub-pixel horizontal positioning.
	bool UseFreetypeSubpixel;   // Use Freetype's subpixel renderer, instead of emulating it ourselves. This only looks good when SnapHorzText is true.
	                            // This not only determines layout behaviour, but also how our subpixel glyphs are rasterized.
	//bool						EmulateGammaBlending;	// Only applicable when EnableSRGBFramebuffer = true, this tries to emulate gamma-space blending. You would turn this on to get consistent blending on all devices. FAILED EXPERIMENT - BAD IDEA.
	float     SubPixelTextGamma;   // Tweak freetype's gamma when doing sub-pixel text rendering. Should be no need to use anything other than 1.0
	float     WholePixelTextGamma; // Tweak freetype's gamma when doing whole-pixel text rendering. Should be no need to use anything other than 1.0
	float     EpToPixel;           // Eye Pixel to Pixel.
	uint32_t  CaretBlinkTimeMS;    // Milliseconds between blinks of the text caret
	TextureID MaxTextureID;        // Used to test texture ID wrap-around. Were it not for testing, this could be 2^32 - 1
	Color     ClearColor;          // glClearColor
	String    CacheDir;            // Root directory where we store font caches, etc. Overridable with InitParams

	// Debugging flags. Enabling these should make debugging easier.
	// Some of them may turn out to have a small enough performance hit that you can
	// leave them turned on always.
	// NOPE.. it's just too confusing to have this optional. It's always on.
	//bool						DebugZeroClonedChildList;	// During a document clone, zero out ChildByInternalID before populating. This will ensure that gaps are NULL instead of random memory.

	// Used to reduce console clutter, when showing messages about missing style variables
	std::atomic<uint32_t> LastStyleLookupFailHash;

	cheapvec<DocGroup*>   Docs;           // Only Main thread is allowed to touch this.
	TQueue<DocGroup*>     DocAddQueue;    // Documents requesting addition
	TQueue<DocGroup*>     DocRemoveQueue; // Documents requesting removal
	TQueue<OriginalEvent> UIEventQueue;   // Global event queue, consumed by the one-and-only UI thread
	TQueue<Job>           JobQueue;       // Global job queue, consumed by the worker thread pool
	FontStore*            FontStore;      // All fonts known to the system.
	GlyphCache*           GlyphCache;     // This might have to move into a less global domain.

	std::atomic<bool>        ExitSignalled;
	std::vector<std::thread> WorkerThreads;
	std::thread              UIThread; // Only used on Windows desktop
};

// Optional initialization parameters
struct InitParams {
	float  EpToPixel = 0; // Override Eye Pixels to Device Pixels (ends up in GlobalStruct.EpToPixel)
	String CacheDir;      // Override cache directory used for font caches etc.
};

typedef std::function<void(MainEvent ev)> MainCallbackLowLevel;
typedef std::function<void(SysWnd* wnd)>  MainCallback;

XO_API GlobalStruct* Global();
XO_API void          Initialize(const InitParams* init = nullptr);
XO_API void          Shutdown();
XO_API void          RunAppLowLevel(MainCallbackLowLevel mainCallback);
XO_API void          RunApp(MainCallback mainCallback);
XO_API void          AddOrRemoveDocsFromGlobalList();
XO_API void          ParseFail(const char* msg, ...);
XO_API void          StyleVarLookupFailed(const char* var);
XO_API void          Trace(const char* msg, ...);
XO_API void          TimeTrace(const char* msg, ...);
#if XO_PLATFORM_WIN_DESKTOP
XO_API void RunWin32MessageLoop();
#elif XO_PLATFORM_LINUX_DESKTOP
XO_API void RunXMessageLoop();
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
#define XOTRACE_RENDER(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_RENDER(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_LAYOUT_WARNINGS_ENABLE
#define XOTRACE_LAYOUT_WARNING(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LAYOUT_WARNING(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_LAYOUT_VERBOSE_ENABLE
#define XOTRACE_LAYOUT_VERBOSE(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LAYOUT_VERBOSE(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_EVENTS_ENABLE
#define XOTRACE_EVENTS(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_EVENTS(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_LATENCY_ENABLE
#define XOTRACE_LATENCY(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_LATENCY(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_FONTS_ENABLE
#define XOTRACE_FONTS(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_FONTS(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_OS_MSG_QUEUE_ENABLE
#define XOTRACE_OS_MSG_QUEUE(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_OS_MSG_QUEUE(msg, ...) ((void) 0)
#endif

#ifdef XOTRACE_WARNING_ENABLE
#define XOTRACE_WARNING(msg, ...) TimeTrace(msg, ##__VA_ARGS__)
#else
#define XOTRACE_WARNING(msg, ...) ((void) 0)
#endif
}
