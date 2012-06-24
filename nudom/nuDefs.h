#pragma once

#include "nuApiDecl.h"
#include "nuPlatform.h"

class nuDoc;
class nuDomEl;
class nuLayout;
class nuPool;
class nuProcessor;
class nuRenderDoc;
class nuRenderer;
class nuRenderDomEl;
class nuRenderGL;
class nuString;
class nuStyle;
class nuSysWnd;
class nuEvent;

typedef int32 nuPos;
typedef int32 nuInternalID;

static const u32 nuPosShift = 8;

inline int32	nuRealToPos( float real )		{ return int32(real * (1 << nuPosShift)); }
inline int32	nuDoubleToPos( double real )	{ return int32(real * (1 << nuPosShift)); }
inline float	nuPosToReal( int32 pos )		{ return pos * (1.0f / (1 << nuPosShift)); }
inline double	nuPosToDouble( int32 pos )		{ return pos * (1.0 / (1 << nuPosShift)); }

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

#define NU_TAGS_DEFINE \
XX(Body, 1) \
XY(Div) \
XY(END) \

#define XX(a,b) nuTag##a = b,
#define XY(a) nuTag##a,
enum nuTag {
	NU_TAGS_DEFINE
};
#undef XX
#undef XY

struct nuVec2
{
	float x,y;
};
inline nuVec2 NUVEC2(float x, float y) { nuVec2 v = {x,y}; return v; }

struct nuVec3
{
	float x,y,z;
};
inline nuVec3 NUVEC3(float x, float y, float z) { nuVec3 v = {x,y,z}; return v; }

class NUAPI nuBox
{
public:
	nuPos	Left, Right, Top, Bottom;

	nuBox() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBox( nuPos left, nuPos top, nuPos right, nuPos bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}

	void	SetInt( int32 left, int32 top, int32 right, int32 bottom );
	nuPos	Width() const	{ return Right - Left; }
	nuPos	Height() const	{ return Bottom - Top; }
	void	Offset( int32 x, int32 y ) { Left += x; Right += x; Top += y; Bottom += y; }
};

class NUAPI nuBoxF
{
public:
	float	Left, Right, Top, Bottom;

	nuBoxF() : Left(0), Right(0), Top(0), Bottom(0) {}
	nuBoxF( float left, float top, float right, float bottom ) : Left(left), Right(right), Top(top), Bottom(bottom) {}
};

struct nuStyleID
{
	uint32	StyleID;
	explicit nuStyleID( uint32 id ) : StyleID(id) {}
	operator uint32 () const { return StyleID; }
};

NUAPI void			nuInitialize();
NUAPI void			nuShutdown();
NUAPI nuStyle**		nuDefaultTagStyles();
NUAPI void			nuQueueRender( nuProcessor* proc );
NUAPI void			nuParseFail( const char* msg, ... );
NUAPI void			NUTRACE( const char* msg, ... );

