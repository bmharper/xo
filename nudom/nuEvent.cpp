#include "pch.h"
#include "nuEvent.h"

NUAPI bool nuEventHandler_LambdaStaticFunc(const nuEvent& ev)
{
	nuEventHandlerLambda* lambda = reinterpret_cast<nuEventHandlerLambda*>(ev.Context);
	return (*lambda)(ev);
}

nuEvent::nuEvent()
{
	Processor = NULL;
	Context = NULL;
	Target = NULL;
	Type = nuEventMouseMove;
	PointCount = 0;
	memset( Points, 0, sizeof(Points) );
}

nuEvent::~nuEvent()
{
}

void nuEvent::MakeWindowSize( int w, int h )
{
	Type = nuEventWindowSize;
	Points[0].x = (float) w;
	Points[0].y = (float) h;
}

nuEventHandler::nuEventHandler()
{
	Mask = 0;
	Flags = 0;
	Context = NULL;
	Func = NULL;
}

nuEventHandler::~nuEventHandler()
{
	if ( IsLambda() )
		delete reinterpret_cast<nuEventHandlerLambda*>(Context);
}
