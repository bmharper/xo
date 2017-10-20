#pragma once

#include "VirtualDom.h"

namespace xo {
namespace vdom {

enum class PatchOp {
	Delete,
	Insert,
};

// Compute the difference between two sequences of Nodes, which we call a and b.
// Emit operations that need to be executed on sequence 'a', so that it becomes 'b'.
XO_API void Diff(size_t na, Node** a, size_t nb, Node** b, std::function<void(PatchOp op, size_t pos, size_t len, Node* const* first)> apply);

// This was made for testing the diff core. The diff core is capable of diffing anything that
// has an equality operator and a hash function, so testing it on text is an easy way to verify
// the algorithm.
// Returns the result of the diff, which ought to be equal to b.
// ops gets a negative number added when a deletion occurs, and a positive number when an insertion occurs
XO_API std::string DiffTest(const char* a, const char* b, std::vector<int>& ops);

} // namespace vdom
} // namespace xo