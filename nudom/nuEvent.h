#pragma once

#include "nuDefs.h"

// It will be good if we can keep these inside 32 bits, for easy masking of handlers. If not, just use as many 32-bit words as necessary.
enum nuEvents
{
	nuEventTouch		= BIT(0),
	nuEventMouseMove	= BIT(1),
	nuEventWindowSize	= BIT(2),
	nuEventTimer		= BIT(3),
};

/* User interface event (keyboard, mouse, touch, etc).
*/
class NUAPI nuEvent
{
public:
	nuDocGroup*		DocGroup;
	void*			Context;
	nuDomEl*		Target;
	nuEvents		Type;
	int				PointCount;					// Mouse = 1	Touch >= 1
	nuVec2f			Points[NU_MAX_TOUCHES];

	nuEvent();
	~nuEvent();

	void MakeWindowSize( int w, int h );
};

typedef std::function<bool(const nuEvent& ev)> nuEventHandlerLambda;

typedef bool (*nuEventHandlerF)(const nuEvent& ev);

NUAPI bool nuEventHandler_LambdaStaticFunc(const nuEvent& ev);

enum nuEventHandlerFlags
{
	nuEventHandlerFlag_IsLambda = 1,
};

class NUAPI nuEventHandler
{
public:
	uint32				Mask;
	uint32				Flags;
	void*				Context;
	nuEventHandlerF		Func;

			nuEventHandler();
			~nuEventHandler();

	bool	Handles( nuEvents ev ) const	{ return !!(Mask & ev); }
	bool	IsLambda() const				{ return !!(Flags & nuEventHandlerFlag_IsLambda); }
	void	SetLambda()						{ Flags |= nuEventHandlerFlag_IsLambda; }
};
