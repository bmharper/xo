#pragma once

namespace xo {
namespace vdom {

struct Attrib {
	const char* Name;
	const char* Val;
};

struct Node {
public:
	const char* Name;
	const char* Val;

	size_t NChild;
	Node** Children;

	size_t  NAttrib;
	Attrib* Attribs;

	bool IsText() const { return Name == nullptr; }
};

} // namespace vdom
} // namespace xo