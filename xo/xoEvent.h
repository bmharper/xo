#pragma once

#include "xoDefs.h"

// It will be good if we can keep these inside 32 bits, for easy masking of handlers. If not, just use as many 32-bit words as necessary.
enum xoEvents
{
	xoEventWindowSize	= BIT(0),
	xoEventTimer		= BIT(1),
	xoEventGetFocus		= BIT(2),
	xoEventLoseFocus	= BIT(3),
	xoEventTouch		= BIT(4),
	xoEventClick		= BIT(5),
	xoEventDblClick		= BIT(6),
	xoEventMouseMove	= BIT(7),
	xoEventMouseEnter	= BIT(8),
	xoEventMouseLeave	= BIT(9),
	xoEventMouseDown	= BIT(10),
	xoEventMouseUp		= BIT(11),
};

enum xoMouseButton
{
	xoMouseButtonNull = 0,
	xoMouseButtonLeft = 1,
	xoMouseButtonMiddle = 2,
	xoMouseButtonRight = 3,
	xoMouseButtonX1 = 4,		// Windows X button 1 (back)
	xoMouseButtonX2 = 5,		// Windows X button 2 (forward)
	xoMouseButtonX3 = 6,		// Windows X button 3 (not sure if this ever exists)
	xoMouseButtonX4 = 7,		// Windows X button 4 (not sure if this ever exists)
};

/* User interface event (keyboard, mouse, touch, etc).
*/
class XOAPI xoEvent
{
public:
	xoDoc*			Doc			= nullptr;
	void*			Context		= nullptr;
	xoDomEl*		Target		= nullptr;
	xoEvents		Type		= xoEventMouseMove;
	xoMouseButton	Button		= xoMouseButtonNull;
	int				PointCount	= 0;					// Mouse = 1	Touch >= 1
	xoVec2f			Points[XO_MAX_TOUCHES];

			xoEvent();
			~xoEvent();

	void	MakeWindowSize( int w, int h );
};

// This is the event that the Windowing system will post onto the single event queue.
// Because there is only one event queue, it needs to know the destination DocGroup.
class XOAPI xoOriginalEvent
{
public:
	xoDocGroup*		DocGroup;
	xoEvent			Event;
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
