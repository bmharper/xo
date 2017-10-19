#pragma once

#include "VirtualDom.h"

namespace xo {
namespace vdom {

enum class PatchOp {
	Delete,
	Insert,
};

void   Diff(Node* a, Node* b, std::function<void(PatchOp op, Node* a, Node* b)> apply);
XO_API std::string DiffTest(const char* a, const char* b, std::vector<int>& ops);

} // namespace vdom
} // namespace xo