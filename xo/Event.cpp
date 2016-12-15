#include "pch.h"
#include "Event.h"

namespace xo {

XO_API bool EventHandler_LambdaStaticFunc(const Event& ev) {
	EventHandlerLambda* lambda = reinterpret_cast<EventHandlerLambda*>(ev.Context);
	return (*lambda)(ev);
}

Event::Event() {
	memset(Points, 0, sizeof(Points));
}

Event::~Event() {
}

void Event::MakeWindowSize(int w, int h) {
	Type        = EventWindowSize;
	Points[0].x = (float) w;
	Points[0].y = (float) h;
}

EventHandler::EventHandler() {
}

EventHandler::~EventHandler() {
	if (IsLambda())
		delete reinterpret_cast<EventHandlerLambda*>(Context);
}
}
