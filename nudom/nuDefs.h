#pragma once

// This is the bottom-level include file that many other nudom headers include. It should
// strive to remain quite small.

#include "nuPlatform.h"

class nuDomEl;
class nuDomNode;
class nuDomText;
class nuDoc;
class nuEvent;
class nuImage;
class nuImageStore;
class nuLayout;
class nuPool;
class nuDocGroup;
class nuRenderDoc;
class nuRenderer;
class nuRenderDomEl;
class nuRenderDomNode;
class nuRenderDomText;
struct nuRenderCharEl;
class nuRenderBase;
class nuRenderGL;
class nuRenderDX;
class nuString;
class nuStringTable;
class nuStyle;
class nuSysWnd;
class nuFont;
class nuFontStore;
class nuGlyphCache;
class nuTextureAtlas;
#ifndef NU_MAT4F_DEFINED
class nuMat4f;
#endif

typedef int32 nuPos;								// fixed-point position
static const u32 nuPosShift = 8;					// 24:8 fixed point coordinates used during layout
static const u32 nuPosMask = (1 << nuPosShift) - 1;	// 255

// An ID that is internal to nudom - i.e. it is not controllable by external code.
// This ID is an integer that you can use to reference a DOM element. These IDs are recycled.
typedef int32 nuInternalID;		
static const nuInternalID nuInternalIDNull = 0;		// Zero is always an invalid DOM element ID
static const nuInternalID nuInternalIDRoot = 1;		// The root of the DOM tree always has ID = 1

typedef int32 nuFontID;
static const nuFontID nuFontIDNull = 0;				// Zero is always an invalid Font ID

// Handle to a texture that is (maybe) resident in the graphics driver.
// nudom supports the concept of the graphics device being "lost", so just because you have
// a non-zero nuTextureID, does not mean that the ID is valid. Prior to drawing the scene,
// the texture loading functions must check whether the ID is still valid or not.
// Note that there is a lot of unsigned arithmetic used by the texture management facilities,
// so this data type must remain unsigned.
typedef uint32 nuTextureID;
static const nuTextureID nuTextureIDNull = 0;		// Zero is always an invalid Texture ID

// Maximum number of texture units that we will try to use
static const u32 nuMaxTextureUnits = 8;

inline int32	nuRealToPos( float real )		{ return int32(real * (1 << nuPosShift)); }
inline int32	nuDoubleToPos( double real )	{ return int32(real * (1 << nuPosShift)); }
inline float	nuPosToReal( int32 pos )		{ return pos * (1.0f / (1 << nuPosShift)); }
inline double	nuPosToDouble( int32 pos )		{ return pos * (1.0 / (1 << nuPosShift)); }
inline int32	nuPosRound( int32 pos )			{ return pos + (1 << (nuPosShift-1)) & ~nuPosMask; }
inline float	nuRound( float real )			{ return floor(real + 0.5f); }

template<typename T>	T nuClamp( T v, T vmin, T vmax )	{ return (v < vmin) ? vmin : (v > vmax) ? vmax : v; }
template<typename T>	T nuMin( T a, T b )					{ return a < b ? a : b; }
template<typename T>	T nuMax( T a, T b )					{ return a < b ? b : a; }

enum nuCloneFlags
{
	nuCloneFlagEvents = 1,		// Include events in clone
};

static const int NU_MAX_TOUCHES = 10;

enum nuMainEvent
{
	nuMainEventInit = 1,
	nuMainEventShutdown,
};

enum nuRenderResult
{
	// SYNC-JAVA
	nuRenderResultNeedMore,
	nuRenderResultIdle
};

#define NU_TAGS_DEFINE \
XX(Body, 1) \
XY(Div) \
XY(Text) \
XY(END) \

#define XX(a,b) nuTag##a = b,
#define XY(a) nuTag##a,
enum nuTag {
	NU_TAGS_DEFINE
};
#undef XX
#undef XY

//struct nuVec2
//{
//	float x,y;
//};
inline nuVec2f NUVEC2(float x, float y) { return nuVec2f(x,y); }

//struct nuVec3
//{
//	float x,y,z;
//};
inline nuVec3f NUVEC3(float x, float y, float z) { return nuVec3f(x,y,z); }

//struct nuVec4
//{
//	float x,y,z,w;
//};
inline nuVec4f NUVEC4(float x, float y, float z, float w) { return nuVec4f(x,y,z,w); }

class NUAPI nuPoint
{
public:
	nuPos	X, Y;

	nuPoint() : X(0), Y(0) {}
	nuPoint( nuPos x, nuPos y ) : X(x), Y(y) {}

	void	SetInt( int32 x, int32 y ) { X = nuRealToPos((float) x); Y = nuRealToPos((float) y); }
};

/*
Why does this class have a copy constructor and assignment operator?
Without those, we get data alignment exceptions (signal 7) when running on my Galaxy S3.
I tried explicitly raising the alignment of nuBox to 8 and 16 bytes, but that did not help.
Unfortunately I have not yet had the energy to open up the assembly and see what the compiler is doing wrong.
This is documented inside nudom/docs/android.md
*/
class NUAPI nuBox
{
public:
	nuPos	Left, Right, Top, Bottom;

	nuBox() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBox( const nuBox& b ) : Left(b.Left), Right(b.Right), Top(b.Top), Bottom(b.Bottom) {}
	nuBox( nuPos left, nuPos top, nuPos right, nuPos bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}

#ifdef _WIN32
	nuBox( RECT r ) : Left(r.left), Right(r.right), Top(r.top), Bottom(r.bottom) {}
	operator RECT() const { RECT r = {Left, Top, Right, Bottom}; return r; }
#endif

	void	SetInt( int32 left, int32 top, int32 right, int32 bottom );
	void	ExpandToFit( const nuBox& expando );
	void	ClampTo( const nuBox& clamp );

	nuPos	Width() const							{ return Right - Left; }
	nuPos	Height() const							{ return Bottom - Top; }
	void	Offset( int32 x, int32 y )				{ Left += x; Right += x; Top += y; Bottom += y; }
	bool	IsInsideMe( const nuPoint& p ) const	{ return p.X >= Left && p.Y >= Top && p.X < Right && p.Y < Bottom; }
	bool	IsAreaZero() const						{ return Width() == 0 || Height() == 0; }

	bool operator==( const nuBox& b ) { return Left == b.Left && Right == b.Right && Top == b.Top && Bottom == b.Bottom; }
	bool operator!=( const nuBox& b ) { return !(*this == b); }

	// $NU_GCC_ALIGN_BUG
	nuBox&	operator=( const nuBox& b ) { Left = b.Left; Right = b.Right; Top = b.Top; Bottom = b.Bottom; return *this; }
};

class NUAPI nuBoxF
{
public:
	float	Left, Right, Top, Bottom;

	nuBoxF() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBoxF( float left, float top, float right, float bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}
};

struct NUAPI nuRGBA
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
	static nuRGBA RGBA(uint8 r, uint8 g, uint8 b, uint8 a) { nuRGBA c; c.r = r; c.g = g; c.b = b; c.a = a; return c; }
};

#define NURGBA(r, g, b, a) ((a) << 24 | (b) << 16 | (g) << 8 | (r))

// This is non-premultipled alpha
struct NUAPI nuColor
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

	void	Set( uint8 _r, uint8 _g, uint8 _b, uint8 _a ) { r = _r; g = _g; b = _b; a = _a; }
	uint32	GetRGBA() const { nuRGBA x; x.r = r; x.g = g; x.b = b; x.a = a; return x.u; }

	bool	operator==( const nuColor& x ) const { return u == x.u; }
	bool	operator!=( const nuColor& x ) const { return u != x.u; }

	static bool		Parse( const char* s, intp len, nuColor& v );
	static nuColor	RGBA( uint8 _r, uint8 _g, uint8 _b, uint8 _a )		{ nuColor c; c.Set(_r,_g,_b,_a); return c; }
	static nuColor	Make( uint32 _u )									{ nuColor c; c.u = _u; return c; }
};

NUAPI float	nuSRGB2Linear( uint8 srgb );
NUAPI uint8	nuLinear2SRGB( float linear );

struct nuStyleID
{
	uint32		StyleID;

				nuStyleID()				: StyleID(0)	{}
	explicit	nuStyleID( uint32 id )	: StyleID(id)	{}

	operator	uint32 () const { return StyleID; }
};

struct nuJob
{
	void*	JobData;
	void (*JobFunc)( void* jobdata );
};

struct NUAPI nuRenderStats
{
	uint32	Clone_NumEls;		// Number of DOM elements cloned

	void Reset();
};

enum nuTexFormat
{
	nuTexFormatInvalid = 0,
	nuTexFormatRGBA8 = 1,
	nuTexFormatGrey8 = 2
};

NUAPI size_t nuTexFormatChannelCount( nuTexFormat f );
NUAPI size_t nuTexFormatBytesPerChannel( nuTexFormat f );
NUAPI size_t nuTexFormatBytesPerPixel( nuTexFormat f );

/* Base of all textures
This structure must remain zero-initializable
Once a texture has been uploaded, you may not change width, height, or channel count.
*/
class NUAPI nuTexture
{
public:
	uint32		TexWidth;
	uint32		TexHeight;
	nuBox		TexInvalidRect;		// Invalid rectangle, in integer texel coordinates.
	nuTextureID	TexID;				// ID of texture in renderer.
	nuTexFormat	TexFormat;
	void*		TexData;
	int			TexStride;

			nuTexture()					{ TexWidth = TexHeight = 0; TexInvalidRect = nuBox(0,0,0,0); TexFormat = nuTexFormatInvalid; TexID = nuTextureIDNull; TexData = NULL; TexStride = 0; }
	void	TexInvalidate()				{ TexInvalidRect = nuBox(0, 0, TexWidth, TexHeight); }
	void	TexValidate()				{ TexInvalidRect = nuBox(0, 0, 0, 0); }
	void*	TexDataAt( int x, int y )	{ return ((char*) TexData) + y * TexStride + x * nuTexFormatBytesPerPixel(TexFormat); }
	void*	TexDataAtLine( int y )		{ return ((char*) TexData) + y * TexStride; }
	size_t	TexBytesPerPixel() const	{ return nuTexFormatBytesPerPixel(TexFormat); }
	void	FlipVertical();
};

// Base of GL and DX shader programs
class nuProgBase
{
public:
};

enum nuShaders
{
	nuShaderInvalid,
	nuShaderFill,
	nuShaderFillTex,
	nuShaderRect,
	nuShaderTextRGB,
	nuShaderTextWhole
	// We may someday want to have slots down here available for application-defined custom shaders
};

// A single instance of this is accessible via nuGlobal()
struct nuGlobalStruct
{
	int							TargetFPS;
	int							NumWorkerThreads;		// Read-only. Set during nuInitialize().
	bool						PreferOpenGL;			// Prefer OpenGL over DirectX. If this is true, then on Windows OpenGL will be tried first.
	bool						EnableVSync;			// This is only respected during device initialization, so you must set it at application start. It raises latency noticeably. This has no effect on DirectX windowed rendering.
	bool						EnableSubpixelText;		// Enable sub-pixel text rendering. Assumes pixels are the standard RGB layout. Enabled by default on Windows desktop only.
	bool						EnableSRGBFramebuffer;	// Enable sRGB framebuffer (implies linear blending)
	//bool						EmulateGammaBlending;	// Only applicable when EnableSRGBFramebuffer = true, this tries to emulate gamma-space blending. You would turn this on to get consistent blending on all devices.
	float						SubPixelTextGamma;		// Tweak freetype's gamma when doing sub-pixel text rendering.
	float						WholePixelTextGamma;	// Tweak freetype's gamma when doing whole-pixel text rendering.
	nuTextureID					MaxTextureID;			// Used to test texture ID wrap-around. Were it not for testing, this could be 2^32 - 1
	nuColor						ClearColor;				// glClearColor

	// Debugging flags. Enabling these should make debugging easier.
	// Some of them may turn out to have a small enough performance hit that you can
	// leave them turned on always.
	// NOPE.. it's just too confusing to have this optional. It's always on.
	//bool						DebugZeroClonedChildList;	// During a document clone, zero out ChildByInternalID before populating. This will ensure that gaps are NULL instead of random memory.

	pvect<nuDocGroup*>			Docs;				// Only Main thread is allowed to touch this.
	TAbcQueue<nuDocGroup*>		DocAddQueue;		// Documents requesting addition
	TAbcQueue<nuDocGroup*>		DocRemoveQueue;		// Documents requesting removal
	TAbcQueue<nuEvent>			EventQueue;			// Global event queue, consumed by the one-and-only UI thread
	TAbcQueue<nuJob>			JobQueue;			// Global job queue, consumed by the worker thread pool
	nuFontStore*				FontStore;			// All fonts known to the system.
	nuGlyphCache*				GlyphCache;			// This might have to move into a less global domain.
};

NUAPI nuGlobalStruct*	nuGlobal();
NUAPI void				nuInitialize();
NUAPI void				nuShutdown();
NUAPI void				nuProcessDocQueue();
NUAPI void				nuParseFail( const char* msg, ... );
NUAPI void				NUTRACE( const char* msg, ... );
NUAPI void				NUTIME( const char* msg, ... );
#if NU_PLATFORM_WIN_DESKTOP
NUAPI void				nuRunWin32MessageLoop();
#endif

// Various tracing options. Uncomment these to enable tracing of that class of events.
//#define NUTRACE_RENDER_ENABLE
//#define NUTRACE_LAYOUT_ENABLE
//#define NUTRACE_EVENTS_ENABLE
//#define NUTRACE_LATENCY_ENABLE

#ifdef NUTRACE_RENDER_ENABLE
	#define NUTRACE_RENDER(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_RENDER(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_LAYOUT_ENABLE
	#define NUTRACE_LAYOUT(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_LAYOUT(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_EVENTS_ENABLE
	#define NUTRACE_EVENTS(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_EVENTS(msg, ...) ((void)0)
#endif

#ifdef NUTRACE_LATENCY_ENABLE
	#define NUTRACE_LATENCY(msg, ...) NUTIME(msg, ##__VA_ARGS__)
#else
	#define NUTRACE_LATENCY(msg, ...) ((void)0)
#endif
