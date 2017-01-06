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
	EventKeyDown    = 4096,
	EventKeyUp      = 8192,
	EventKeyChar    = 16384,
	EventDestroy    = 32768, // DOM node is being removed from document
};

enum class Button {
	Null        = 0,
	MouseLeft   = 1,
	MouseMiddle = 2,
	MouseRight  = 3,
	MouseX1     = 4, // Windows X button 1 (back)
	MouseX2     = 5, // Windows X button 2 (forward)
	MouseX3     = 6, // Windows X button 3 (not sure if this ever exists)
	MouseX4     = 7, // Windows X button 4 (not sure if this ever exists)
	Key0,
	Key9 = Key0 + 9,
	KeyA,
	KeyZ = KeyA + 25,
	KeyBack,
	KeySpace,
	KeyTab,
	KeyEscape,
	KeyInsert,
	KeyDelete,
	KeyHome,
	KeyEnd,
	KeyPageUp,
	KeyPageDown,
	KeyNumLeft,
	KeyNumRight,
	KeyNumUp,
	KeyNumDown,
	KeyNumInsert,
	KeyNumDelete,
	KeyNumPlus,
	KeyNumMinus,
	KeyNumMultiply,
	KeyNumDivide,
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
	Button              Button       = Button::Null;
	int32_t             KeyChar      = 0;          // Unicode code point of a key message
	int                 PointCount   = 0;          // Mouse = 1	Touch >= 1
	Vec2f               PointsAbs[XO_MAX_TOUCHES]; // Points in pixels, relative to viewport top-left
	Vec2f               PointsRel[XO_MAX_TOUCHES]; // Points relative to Target's content-box top-left

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
