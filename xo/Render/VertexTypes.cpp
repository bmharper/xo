#include "pch.h"
#include "VertexTypes.h"

namespace xo {

XO_API size_t VertexSize(VertexType t) {
	switch (t) {
	case VertexType_NULL: return 0;
	case VertexType_PTC: return sizeof(Vx_PTC);
	case VertexType_PTCV4: return sizeof(Vx_PTCV4);
	case VertexType_Uber: return sizeof(Vx_Uber);
	default:
		XO_TODO;
		return 0;
	}
}
}
