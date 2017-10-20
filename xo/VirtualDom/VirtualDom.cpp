#include "pch.h"
#include "VirtualDom.h"

namespace xo {
namespace vdom {

void Node::ComputeHashTree() {
	uint32_t h = FNV1_32A_INIT;

	for (size_t i = 0; i < NChild; i++) {
		Children[i]->ComputeHashTree();
		h = fnv_32a_buf(&Children[i]->Hash, sizeof(Hash), h);
	}

	for (size_t i = 0; i < NAttrib; i++) {
		h = fnv_32a_str(Attribs[i].Name, h);
		h = fnv_32a_str(Attribs[i].Val, h);
	}

	if (Name)
		h = fnv_32a_str(Name, h);

	if (Val)
		h = fnv_32a_str(Val, h);

	Hash = h;
}

} // namespace vdom
} // namespace xo
