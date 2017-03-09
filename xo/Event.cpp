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

bool ControlKeyStates::IsKeyDown(Button btn) const {
	switch (btn) {
	case Button::KeyShift: return LShift || RShift;
	case Button::KeyLShift: return LShift;
	case Button::KeyRShift: return RShift;
	case Button::KeyMenu: return LMenu || RMenu;
	case Button::KeyLMenu: return LMenu;
	case Button::KeyRMenu: return RMenu;
	case Button::KeyCtrl: return LCtrl || RCtrl;
	case Button::KeyLCtrl: return LCtrl;
	case Button::KeyRCtrl: return RCtrl;
	case Button::KeyWindows: return LWindows || RWindows;
	case Button::KeyLWindows: return LWindows;
	case Button::KeyRWindows: return RWindows;
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

bool Event::IsControlKeyDown(xo::Button btn) const {
	return ControlKeys.IsKeyDown(btn);
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
