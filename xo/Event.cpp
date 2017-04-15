#include "pch.h"
#include "Event.h"

namespace xo {

XO_API int ButtonToMouseNumber(Button b) {
	int i = (int) b - (int) Button::MouseLeft;
	if (i >= 0 && i < NumMouseButtons)
		return i;
	else
		return -1;
}

XO_API Button AsciiToButton(char c) { 
	if (c >= 'a' && c <= 'z')
		return (Button) ((int) (c - 'a') + (int) Button::KeyA);

	if (c >= 'Z' && c <= 'Z')
		return (Button) ((int) (c - 'A') + (int) Button::KeyA);

	if (c >= '0' && c <= '9')
		return (Button) ((int) (c - '0') + (int) Button::Key0);

	return Button::Null;
}

bool ButtonStates::IsPressed(Button btn) const {
	switch (btn) {
	case Button::KeyShift: return LShift || RShift;
	case Button::KeyLShift: return LShift;
	case Button::KeyRShift: return RShift;
	case Button::KeyAlt: return LAlt || RAlt;
	case Button::KeyLAlt: return LAlt;
	case Button::KeyRAlt: return RAlt;
	case Button::KeyCtrl: return LCtrl || RCtrl;
	case Button::KeyLCtrl: return LCtrl;
	case Button::KeyRCtrl: return RCtrl;
	case Button::KeyWindows: return LWindows || RWindows;
	case Button::KeyLWindows: return LWindows;
	case Button::KeyRWindows: return RWindows;
	case Button::MouseLeft: return MouseLeft;
	case Button::MouseMiddle: return MouseMiddle;
	case Button::MouseRight: return MouseRight;
	case Button::MouseX1: return MouseX1;
	case Button::MouseX2: return MouseX2;
	default: return false;
	}
}

XO_API void EventHandler_LambdaStaticFunc0(Event& ev) {
	EventHandlerLambda0* lambda = reinterpret_cast<EventHandlerLambda0*>(ev.Context);
	(*lambda)();
}

XO_API void EventHandler_LambdaStaticFunc1(Event& ev) {
	EventHandlerLambda1* lambda = reinterpret_cast<EventHandlerLambda1*>(ev.Context);
	(*lambda)(ev);
}

Event::Event() {
	memset(PointsRel, 0, sizeof(PointsRel));
	memset(PointsAbs, 0, sizeof(PointsAbs));
}

Event::~Event() {
}

void Event::MakeWindowSize(int w, int h) {
	Type           = EventWindowSize;
	PointsAbs[0].x = (float) w;
	PointsAbs[0].y = (float) h;
}

bool Event::IsPressed(xo::Button btn) const {
	return ButtonStates.IsPressed(btn);
}

EventHandler::EventHandler() {
}

EventHandler::~EventHandler() {
	if (IsLambda0())
		delete reinterpret_cast<EventHandlerLambda0*>(Context);
	else if (IsLambda1())
		delete reinterpret_cast<EventHandlerLambda1*>(Context);
}
}
