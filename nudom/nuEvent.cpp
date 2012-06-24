#include "pch.h"
#include "nuEvent.h"

nuEvent::nuEvent()
{
	Type = nuEventMouseMove;
	Context = NULL;
	Target = NULL;
	PointCount = 0;
	memset( Points, 0, sizeof(Points) );
}

nuEvent::~nuEvent()
{
}
