#include "pch.h"
#include "nuVertexTypes.h"

NUAPI size_t nuVertexSize( nuVertexType t )
{
	switch ( t )
	{
	case nuVertexType_NULL:		return 0;
	case nuVertexType_PTC:		return sizeof(nuVx_PTC);
	case nuVertexType_PTCV4:	return sizeof(nuVx_PTCV4);
	default:
		NUTODO;
		return 0;
	}
}