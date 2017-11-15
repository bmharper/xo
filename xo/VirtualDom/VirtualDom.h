#pragma once

namespace xo {
class DomEl;
class DomNode;
namespace rx {
class Registry;
}
namespace vdom {

struct Attrib {
	const char* Name;
	const char* Val;
};

struct Node {
public:
	enum class HashMode {
		All,      // Incorporate all state of Node and child nodes into hash
		NameOnly, // Only hash 'Name' (aka node type). Very loose diffing - requires walking down trees.
	};
	uint32_t    Hash;
	const char* Name;
	const char* Val; // Used only for Text nodes

	size_t NChild;
	Node** Children;

	size_t  NAttrib;
	Attrib* Attribs;

	bool IsNode() const { return Name != nullptr; }
	bool IsText() const { return Name == nullptr; }

	bool EqualAttribs(const Node& b) const;

	// Recursively compute Hash for this node, and all of it's children
	void ComputeHashTree(HashMode mode);
};

/* Converter from Virtual DOM to Real DOM.

This is used in conjunctions with the virtual dom differ. It receives
a set of 'patch' commands from the differ, and turns those patches into
real DOM creation statements.

Through the Registry, this class knows the names of controls, so
that when it sees <foobar>...</foobar> in the virtual dom, it knows
to create a FooBar control.
*/
class VDomToRealDom {
public:
	rx::Registry* Registry = nullptr;

	VDomToRealDom(rx::Registry* registry) : Registry(registry) {}

	// Returns error message or empty string on success
	String CreateChild(DomNode* owner, size_t pos, const vdom::Node* newChild);
};

} // namespace vdom
} // namespace xo