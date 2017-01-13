#include "pch.h"
#include "Event.h"

namespace xo {

XO_API void EventHandler_LambdaStaticFunc(Event& ev) {
	EventHandlerLambda* lambda = reinterpret_cast<EventHandlerLambda*>(ev.Context);
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

EventHandler::EventHandler() {
}

EventHandler::~EventHandler() {
	if (IsLambda())
		delete reinterpret_cast<EventHandlerLambda*>(Context);
}
}
