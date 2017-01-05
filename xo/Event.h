#pragma once
#include "Defs.h"

namespace xo {

// It will be good if we can keep these inside 32 bits, for easy masking of handlers.
// If not, just use as many 64-bit words as necessary.
enum Events {
	EventWindowSize = 1,
	EventTimer      = 2,
	EventGetFocus   = 4,
	EventLoseFocus  = 8,
	EventTouch      = 16,
	EventClick      = 32,
	EventDblClick   = 64,
	EventMouseMove  = 128,
	EventMouseEnter = 256,
	EventMouseLeave = 512,
	EventMouseDown  = 1024,
	EventMouseUp    = 2048,
	EventDestroy    = 4096, // DOM node is being removed from document
};

enum MouseButton {
	MouseButtonNull   = 0,
	MouseButtonLeft   = 1,
	MouseButtonMiddle = 2,
	MouseButtonRight  = 3,
	MouseButtonX1     = 4, // Windows X button 1 (back)
	MouseButtonX2     = 5, // Windows X button 2 (forward)
	MouseButtonX3     = 6, // Windows X button 3 (not sure if this ever exists)
	MouseButtonX4     = 7, // Windows X button 4 (not sure if this ever exists)
};

/* User interface event (keyboard, mouse, touch, etc).
*/
class XO_API Event {
public:
	Doc*                Doc          = nullptr;
	void*               Context      = nullptr;
	DomEl*              Target       = nullptr;
	DomText*            TargetText   = nullptr;
	int32_t             TargetChar   = -1;
	const LayoutResult* LayoutResult = nullptr;
	Events              Type         = EventMouseMove;
	MouseButton         Button       = MouseButtonNull;
	int                 PointCount   = 0; // Mouse = 1	Touch >= 1
	Vec2f               Points[XO_MAX_TOUCHES];

	Event();
	~Event();

	void MakeWindowSize(int w, int h);
};

// This is the event that the Windowing system will post onto the single event queue.
// Because there is only one event queue, it needs to know the destination DocGroup.
class XO_API OriginalEvent {
public:
	DocGroup* DocGroup = nullptr;
	Event     Event;
};

typedef std::function<bool(const Event& ev)> EventHandlerLambda;

typedef bool (*EventHandlerF)(const Event& ev);

XO_API bool EventHandler_LambdaStaticFunc(const Event& ev);

enum EventHandlerFlags {
	EventHandlerFlag_IsLambda = 1,
};

class XO_API EventHandler {
public:
	uint64_t      ID              = 0;
	uint32_t      Mask            = 0;
	uint32_t      Flags           = 0;
	uint32_t      TimerPeriodMS   = 0; // Only applicable to Timer event handlers
	uint32_t      TimerLastTickMS = 0; // Only applicable to Timer event handlers. Time in xo::MilliTicks() when we last ticked, truncated to uint32.
	void*         Context         = nullptr;
	EventHandlerF Func            = nullptr;

	EventHandler();
	~EventHandler();

	bool Handles(Events ev) const { return !!(Mask & ev); }
	bool IsLambda() const { return !!(Flags & EventHandlerFlag_IsLambda); }
	void SetLambda() { Flags |= EventHandlerFlag_IsLambda; }
};

struct NodeEventIDPair {
	InternalID NodeID;
	uint64_t   EventID;
};
}
