#pragma once

namespace xo {
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
	uint32_t    Hash;
	const char* Name;
	const char* Val;

	size_t NChild;
	Node** Children;

	size_t  NAttrib;
	Attrib* Attribs;

	bool IsText() const { return Name == nullptr; }

	// Recursively compute Hash for this node, and all of it's children
	void ComputeHashTree();
};

/* Converter from Virtual DOM to Real DOM.

This is used in conjunctions with the virtual dom differ. It receives
a set of 'patch' commands from the differ, and turns those patches into
real DOM creation statements.

This class is also responsible for knowing the names of controls, so
that when it sees <foobar>...</foobar> in the virtual dom, it knows
to create a FooBar control.
*/
class VDomToRealDom {
public:
	rx::Registry* Registry = nullptr;

	VDomToRealDom(rx::Registry* registry) : Registry(registry) {}

	//void ApplyDiff
};

} // namespace vdom
} // namespace xo