#include "pch.h"
#include "VirtualDom.h"
#include "../Dom/DomNode.h"
#include "../Reactive/Registry.h"
#include "../Reactive/Control.h"
#include "../Parse/DocParser.h"

namespace xo {
namespace vdom {

bool Node::EqualAttribs(const Node& b) const {
	if (NAttrib != b.NAttrib)
		return false;

	for (size_t i = 0; i < NAttrib; i++) {
		if (strcmp(Attribs[i].Name, b.Attribs[i].Name) != 0)
			return false;
		if (strcmp(Attribs[i].Val, b.Attribs[i].Val) != 0)
			return false;
	}

	return true;
}

void Node::ComputeHashTree(HashMode mode) {
	uint32_t h = FNV1_32A_INIT;

	if (mode == HashMode::All) {
		if (IsNode()) {
			for (size_t i = 0; i < NChild; i++) {
				Children[i]->ComputeHashTree(mode);
				h = fnv_32a_buf(&Children[i]->Hash, sizeof(Hash), h);
			}

			for (size_t i = 0; i < NAttrib; i++) {
				h = fnv_32a_str(Attribs[i].Name, h);
				h = fnv_32a_str(Attribs[i].Val, h);
			}

			h = fnv_32a_str(Name, h);
		} else {
			h = fnv_32a_str(Val, h);
		}
	} else if (mode == HashMode::NameOnly) {
		if (IsNode()) {
			for (size_t i = 0; i < NChild; i++)
				Children[i]->ComputeHashTree(mode);
			h = fnv_32a_str(Name, h);
		} else {
			h = 1;
		}
	}

	Hash = h;
}

String VDomToRealDom::CreateChild(DomNode* owner, size_t pos, const vdom::Node* newChild) {
	if (newChild->IsText()) {
		// Text is set by recursive update, so for consistency, we don't set it here.
		owner->AddText("");
		return "";
	}

	rx::Control* childControl = Registry->Create(newChild->Name);
	if (childControl) {
		// what now!?!
		// recursively render I guess.
		return "";
	}

	auto tag = DocParser::ParseTag(newChild->Name);
	if (tag == TagNULL)
		return tsf::fmt("Invalid tag '%v'", newChild->Name).c_str();
	owner->AddChild(tag);
	return "";
}

} // namespace vdom
} // namespace xo
