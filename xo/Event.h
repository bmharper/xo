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
	EventRender     = 65536, // Document has finished rendering
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
	KeyBack, // aka Backspace
	KeySpace,
	KeyTab,
	KeyEscape,
	KeyInsert,
	KeyDelete,
	KeyHome,
	KeyEnd,
	KeyPageUp,
	KeyPageDown,
	KeyLeft,
	KeyRight,
	KeyUp,
	KeyDown,
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
	KeyShift, // A pseudo-key, used for checking whether either shift key is down
	KeyLShift,
	KeyRShift,
	KeyAlt, // A pseudo-key, used for checking whether either alt key is down
	KeyLAlt,
	KeyRAlt,
	KeyCtrl, // A pseudo-key, used for checking whether either ctrl key is down
	KeyLCtrl,
	KeyRCtrl,
	KeyWindows, // A pseudo-key, used for checking whether either windows key is down
	KeyLWindows,
	KeyRWindows,
};

enum {
	// Number of mouse buttons that we can represent. See ButtonToMouseNumber.
	NumMouseButtons = (int) Button::MouseX4 - (int) Button::MouseLeft
};

// Returns an integer between 0 and MouseNumberSize - 1, or -1 if this is not a mouse button
XO_API int ButtonToMouseNumber(Button b);

// Convert an ASCII character from a..z, A..Z, 0..9, to a Button. a..z and A..Z are mapped to the same range of buttons.
XO_API Button AsciiToButton(char c);

class XO_API ModifierKeyStates {
public:
	union {
		struct {
			bool LCtrl : 1;
			bool RCtrl : 1;
			bool LAlt : 1;
			bool RAlt : 1;
			bool LWindows : 1;
			bool RWindows : 1;
			bool LShift : 1;
			bool RShift : 1;
		};
		uint32_t All;
	};

	ModifierKeyStates() : All(0) {}
	bool IsKeyDown(Button btn) const;
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
	ModifierKeyStates   ModifierKeys;                     // Modifier key state. These are saved when the message is generated, to guarantee correct timing of keys (in case event is processed after control key is lifted)
	int                 KeyChar    = 0;                   // Unicode code point of a key message. If not a Unicode code point (eg DELETE), then use Button
	int                 PointCount = 0;                   // Mouse = 1	Touch >= 1
	Vec2f               PointsAbs[XO_MAX_TOUCHES];        // Points in pixels, relative to viewport top-left
	Vec2f               PointsRel[XO_MAX_TOUCHES];        // Points relative to Target's content-box top-left
	bool                IsStopPropagationToggled = false; // True if StopPropagation() has been called, and the event must not bubble out to enclosing DOM elements
	bool                IsCancelTimerToggled     = false; // True if CancelTimer() has been called, in which case the timer will be cancelled

	Event();
	~Event();

	void MakeWindowSize(int w, int h);
	void StopPropagation() { IsStopPropagationToggled = true; } // Stop bubbling out to higher DOM elements
	void CancelTimer() { IsCancelTimerToggled = true; }         // Cancel this timer.
	bool IsModifierKeyDown(xo::Button btn) const;               // Returns the state of the control key when the event was fired. See ControlKeyStates for valid control keys
};

// This is the event that the Windowing system will post onto the single event queue.
// Because there is only one event queue, it needs to know the destination DocGroup.
class XO_API OriginalEvent {
public:
	DocGroup* DocGroup = nullptr;
	Event     Event;
};

typedef std::function<void()>          EventHandlerLambda0; // 0 Parameters
typedef std::function<void(Event& ev)> EventHandlerLambda1; // 1 Parameter

typedef void (*EventHandlerF)(Event& ev);

XO_API void EventHandler_LambdaStaticFunc0(Event& ev);
XO_API void EventHandler_LambdaStaticFunc1(Event& ev);

enum EventHandlerFlags {
	EventHandlerFlag_None      = 0,
	EventHandlerFlag_IsLambda0 = 1, // 0 parameter lambda function
	EventHandlerFlag_IsLambda1 = 2, // 1 parameter lambda function
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
	bool IsLambda0() const { return !!(Flags & EventHandlerFlag_IsLambda0); }
	bool IsLambda1() const { return !!(Flags & EventHandlerFlag_IsLambda1); }
	void SetLambda0() { Flags |= EventHandlerFlag_IsLambda0; }
	void SetLambda1() { Flags |= EventHandlerFlag_IsLambda1; }
};

struct NodeEventIDPair {
	InternalID NodeID;
	uint64_t   EventID;
};
}
