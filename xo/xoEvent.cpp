#include "pch.h"
#include "xoEvent.h"

XOAPI bool xoEventHandler_LambdaStaticFunc(const xoEvent& ev)
{
	xoEventHandlerLambda* lambda = reinterpret_cast<xoEventHandlerLambda*>(ev.Context);
	return (*lambda)(ev);
}

xoEvent::xoEvent()
{
	memset( Points, 0, sizeof(Points) );
}

xoEvent::~xoEvent()
{
}

void xoEvent::MakeWindowSize( int w, int h )
{
	Type = xoEventWindowSize;
	Points[0].x = (float) w;
	Points[0].y = (float) h;
}

xoEventHandler::xoEventHandler()
{
	Mask = 0;
	Flags = 0;
	Context = NULL;
	Func = NULL;
}

xoEventHandler::~xoEventHandler()
{
	if ( IsLambda() )
		delete reinterpret_cast<xoEventHandlerLambda*>(Context);
}
