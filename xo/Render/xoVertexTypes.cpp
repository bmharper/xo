#include "pch.h"
#include "xoVertexTypes.h"

XOAPI size_t xoVertexSize(xoVertexType t)
{
	switch (t)
	{
	case xoVertexType_NULL:		return 0;
	case xoVertexType_PTC:		return sizeof(xoVx_PTC);
	case xoVertexType_PTCV4:	return sizeof(xoVx_PTCV4);
	case xoVertexType_Uber:		return sizeof(xoVx_Uber);
	default:
		XOTODO;
		return 0;
	}
}