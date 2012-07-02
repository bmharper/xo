#pragma once

#include "nuDefs.h"

// It will be good if we can keep these inside 32 bits, for easy masking of handlers. If not, just use as many 32-bit words as necessary.
enum nuEvents
{
	nuEventTouch		= BIT(0),
	nuEventMouseMove	= BIT(1),
	nuEventWindowSize	= BIT(2),
};

/* User interface event (keyboard, mouse, touch, etc).
*/
class NUAPI nuEvent
{
public:
	nuProcessor*	Processor;
	void*			Context;
	nuDomEl*		Target;
	nuEvents		Type;
	int				PointCount;					// Mouse = 1	Touch >= 1
	nuVec2			Points[NU_MAX_TOUCHES];

	nuEvent();
	~nuEvent();

	void MakeWindowSize( int w, int h );
};

// The Android ecosystem is just not up to this yet. It is a pity, because lambdas make such good event handlers.
// I haven't even looked at iOS yet.
// typedef std::function<bool(const nuEvent& ev)> nuEventHandler;

typedef bool (*nuEventHandlerF)(const nuEvent& ev);

struct NUAPI nuEventHandler
{
	uint32				Mask;
	void*				Context;
	nuEventHandlerF		Func;

	bool Handles( nuEvents ev ) const { return !!(Mask & ev); }
};
