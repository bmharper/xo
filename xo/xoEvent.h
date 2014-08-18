#pragma once

#include "xoDefs.h"

// It will be good if we can keep these inside 32 bits, for easy masking of handlers. If not, just use as many 32-bit words as necessary.
enum xoEvents
{
	xoEventTouch		= BIT(0),
	xoEventMouseMove	= BIT(1),
	xoEventWindowSize	= BIT(2),
	xoEventTimer		= BIT(3),
	xoEventClick		= BIT(4),
};

/* User interface event (keyboard, mouse, touch, etc).
*/
class XOAPI xoEvent
{
public:
	xoDocGroup*		DocGroup;
	void*			Context;
	xoDomEl*		Target;
	xoEvents		Type;
	int				PointCount;					// Mouse = 1	Touch >= 1
	xoVec2f			Points[XO_MAX_TOUCHES];

	xoEvent();
	~xoEvent();

	void MakeWindowSize( int w, int h );
};

typedef std::function<bool(const xoEvent& ev)> xoEventHandlerLambda;

typedef bool (*xoEventHandlerF)(const xoEvent& ev);

XOAPI bool xoEventHandler_LambdaStaticFunc(const xoEvent& ev);

enum xoEventHandlerFlags
{
	xoEventHandlerFlag_IsLambda = 1,
};

class XOAPI xoEventHandler
{
public:
	uint32				Mask;
	uint32				Flags;
	void*				Context;
	xoEventHandlerF		Func;

			xoEventHandler();
			~xoEventHandler();

	bool	Handles( xoEvents ev ) const	{ return !!(Mask & ev); }
	bool	IsLambda() const				{ return !!(Flags & xoEventHandlerFlag_IsLambda); }
	void	SetLambda()						{ Flags |= xoEventHandlerFlag_IsLambda; }
};
