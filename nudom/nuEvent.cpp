#include "pch.h"
#include "nuEvent.h"

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
