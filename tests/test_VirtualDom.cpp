#include "pch.h"

TESTFUNC(VDomDiff) {
	auto test = [](const char* a, const char* b, std::vector<int> expectOps = {}) {
		std::vector<int> ops;
		auto             r = xo::vdom::DiffTest(a, b, ops);
		TTASSERT(r == b);
		if (expectOps.size() != 0) {
			TTASSERT(ops.size() == expectOps.size());
			for (size_t i = 0; i < expectOps.size(); i++)
				TTASSERT(ops[i] == expectOps[i]);
		}
	};

	test("abc", "abc");
	test("foo abc", "bar abc", {-3, 3});
	test("abc foo", "abc bar", {-3, 3});
	test("the quick brown fox jumps over the lazy dog", "once upon a time, the quick brown fox jumps over the lazy dog", {18});
	test("the quick brown fox jumps over the lazy dog", "the quick brown fox jumps over the lazy dog, and then sat", {14});
	test("the quick brown fox jumps over the lazy dog", "the quick brown fox jumped over the lazy dog");
}

void VDomDiffFuzz(const char* a, const char* b) {
	std::vector<int> ops;
	std::string      r = xo::vdom::DiffTest(a, b, ops);
	XO_ASSERT(r == b);
	tsf::print("r: %v\n", r);
}

// See instructions at top of tundra.lua for how to run afl
void VDomDiffFuzz() {
	std::string all, a, b;
	while (true) {
		char buf[1024];
		char* res = fgets(buf, sizeof(buf), stdin);
		if (!res)
			break;
		all += buf;
	}

	size_t split = all.find(' ');
	a = all.substr(0, split);
	b = all.substr(split + 1);
	VDomDiffFuzz(a.c_str(), b.c_str());
}
