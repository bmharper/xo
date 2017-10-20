#include "pch.h"
#include "Diff.h"

using namespace std;

namespace xo {
namespace vdom {

/* Difference algorithm.

We adjust 'a' so that it becomes 'b'

This is pretty naive - I wanted to implement something from scratch, to give me a better
understanding of the problem. There are definitely faster ways of doing a diff.

How does this work?

This is a recursive function, which does the following:

	1. Find the longest common subsequence (LCS) between a and b. This splits a and b into 3 sections:
	Left of the LCS, the LCS, and right of the LCS. Any one of those 3 sections may be empty.

	2. Recurse on the sections left of the LCS
	3. Recurse on the sections right of the LCS

	4. When reaching terminating conditions of the recursion, emit change operations, which are deletions
	and insertions.

We use a rolling hash function here, copied from rsync, which is simply the sum of the last N tokens.
N is a constant, currently 8, but it might make sense to make it larger. When the smaller of the two
sequences to be compared is less than 2x the window size, then we don't use the rolling hash function,
but just use the token values directly. What are the "token values", and how can we compute the sum
of them? We insist that the caller provide a Hasher object, which takes a single element (aka token),
and returns a 32-bit integer for that element, which we use as it's hash value. When testing the
algorithm on strings, the hash value is simply the value of the character code.

By using rolling hashes, we accelerate the search for substrings, because the rolling hash incorporates
information from a number of adjacent elements.

*/
class DiffCore {
public:
	template <typename T, typename Hasher>
	void Diff(size_t na, size_t nb, const T* a, const T* b, Hasher& hasher, std::function<void(PatchOp op, size_t pos, size_t len, const T* el)> apply) {
		size_t minLen = std::min(na, nb);
		// compute window size based on minLen? Would probably help.

		// Don't use rolling hash unless min(na,nb) >= WindowThreshold
		WindowThreshold = WindowSize * 2;

		// Compute rolling hash for a and b, with window of N, and window of 1
		if (minLen >= WindowThreshold) {
			ComputeRollingHash<T, Hasher>(hasher, WindowSize, na, a, WindowHashA);
			ComputeRollingHash<T, Hasher>(hasher, WindowSize, nb, b, WindowHashB);
		}
		ComputeRollingHash<T, Hasher>(hasher, 1, na, a, SingleHashA);
		ComputeRollingHash<T, Hasher>(hasher, 1, nb, b, SingleHashB);
		DiffCore_R(a, b, 0, na, 0, nb, apply, 0);
	}

private:
	struct Sequence {
		size_t    Begin; // First
		size_t    End;   // One beyond end
		size_t    Len() const { return End - Begin; }
		Sequence& operator+=(size_t i) {
			Begin += i;
			End += i;
			return *this;
		}
	};
	typedef std::vector<uint32_t>     HashList;
	typedef ohash::map<uint32_t, int> THashIndex;

	size_t WindowSize      = 8;
	size_t WindowThreshold = 0;

	// Set of hashes for a window of size WindowSize
	HashList WindowHashA;
	HashList WindowHashB;

	// Set of hashes for a window size of 1
	HashList SingleHashA;
	HashList SingleHashB;

	THashIndex HashIndex;

	template <typename T>
	ssize_t DiffCore_R(const T* a, const T* b, size_t aBegin, size_t aEnd, size_t bBegin, size_t bEnd, std::function<void(PatchOp op, size_t pos, size_t len, const T* el)> apply, size_t patchPosOffset) {
		if (aEnd - aBegin == 0 && bEnd - bBegin == 0) {
			// two empty sequences
			return 0;
		}
		if (aEnd - aBegin == 0) {
			// a is empty, so just insert b
			apply(PatchOp::Insert, patchPosOffset + aBegin, bEnd - bBegin, b + bBegin);
			return bEnd - bBegin;
		}
		if (bEnd - bBegin == 0) {
			// b is empty, so just erase a
			apply(PatchOp::Delete, patchPosOffset + aBegin, aEnd - aBegin, nullptr);
			return -((ssize_t)(aEnd - aBegin));
		}

		Sequence sa, sb;
		auto     minLen = std::min(aEnd - aBegin, bEnd - bBegin);
		if (minLen < WindowSize * 2)
			LongestCommonSubsequence(aEnd - aBegin, bEnd - bBegin, a + aBegin, b + bBegin, &SingleHashA[0] + aBegin, &SingleHashB[0] + bBegin, 1, sa, sb);
		else
			LongestCommonSubsequence(aEnd - aBegin, bEnd - bBegin, a + aBegin, b + bBegin, &WindowHashA[0] + aBegin, &WindowHashB[0] + bBegin, WindowSize, sa, sb);

		ssize_t offset = 0;
		if (sa.Len() == 0) {
			// No common substring, so replace a with b, entirely
			apply(PatchOp::Delete, patchPosOffset + aBegin, aEnd - aBegin, nullptr);
			apply(PatchOp::Insert, patchPosOffset + aBegin, bEnd - bBegin, b + bBegin);
			offset = (ssize_t)(bEnd - bBegin) - (ssize_t)(aEnd - aBegin);
		} else {
			sa += aBegin;
			sb += bBegin;
			offset = DiffCore_R(a, b, aBegin, sa.Begin, bBegin, sb.Begin, apply, patchPosOffset); // left of LCS
			patchPosOffset += offset;
			offset += DiffCore_R(a, b, sa.End, aEnd, sb.End, bEnd, apply, patchPosOffset); // right of LCS
		}
		return offset;
	}

	template <typename T, typename Hasher>
	static void ComputeRollingHash(Hasher& hasher, size_t window, size_t n, const T* v, std::vector<uint32_t>& hashes) {
		hashes.clear();

		// By multiplying the raw hash values by a large prime, we splat them across the uint32 number space,
		// thereby reducing the kind of aliasing that one would otherwise see due to the proximity of ascii
		// characters. For example, 'acc', summed together, is the same as 'abd' (think 1+3+3 = 1+2+4). However,
		// by multiplying those numbers by a large prime, we're effectively redistributing the ascii code points
		// all over the 32-bit integer space, which reduces that sort of collision.
		const uint32_t multiplier = 2654435761U;

		if (window == 1) {
			for (size_t i = 0; i < n; i++)
				hashes.push_back(hasher(v[i]) * multiplier);
		} else {
			uint32_t h = 0;
			for (size_t i = 0; i < n; i++) {
				if (i >= window)
					h -= hasher(v[i - window]) * multiplier;
				h += hasher(v[i]) * multiplier;
				hashes.push_back(h);
			}
		}
	}

	template <typename T>
	void LongestCommonSubsequence(size_t na, size_t nb, const T* a, const T* b, const uint32_t* hashA, const uint32_t* hashB, size_t windowSize, Sequence& _sa, Sequence& _sb) {
		// We ignore rolling hash collisions, and just hope that the odds work in our favour, and
		// even if one match gets lost because of a collision, we'll find an adjacent one.

		// Index B, so that we can quickly search for matching hashes inside it
		HashIndex.clear();
		for (size_t i = 0; i < nb; i++)
			HashIndex.insert({hashB[i], (int) i});

		// Step 2: Find the longest common subsequence
		Sequence sa{0, 0};
		Sequence sb{0, 0};
		size_t   best = 0;

		for (size_t i = 0; i < na; i++) {
			if (na - i < best) {
				// best match is already longer than what remains in 'a', so we cannot possibly find anything longer
				break;
			}
			auto fj = HashIndex.find(hashA[i]);
			if (fj == HashIndex.end())
				continue;
			size_t j = (size_t) fj->second;

			// walk forwards
			size_t minRemain = min(na - i, nb - j);
			size_t k         = 0;
			for (; k < minRemain; k++) {
				if (a[i + k] != b[j + k])
					break;
			}
			size_t r = 0;
			if (windowSize != 1) {
				// walk backwards - necessary for cases where rolling window has caused us to miss something in the beginning of a sequence
				size_t maxBack = min(i, j) + 1;
				r              = 1;
				for (; r < maxBack; r++) {
					if (a[i - r] != b[j - r])
						break;
				}
				r--;
			}
			if (k + r > best) {
				best = k + r;
				sa   = {i - r, i + k};
				sb   = {j - r, j + k};
			}
		}

		_sa = sa;
		_sb = sb;
	}
};

struct NodeHasher {
	uint32_t operator()(Node* n) const {
		return n->Hash;
	}
};

XO_API void Diff(size_t na, Node** a, size_t nb, Node** b, std::function<void(PatchOp op, size_t pos, size_t len, Node* const* first)> apply) {
	for (size_t i = 0; i < na; i++)
		a[i]->ComputeHashTree();
	for (size_t i = 0; i < nb; i++)
		b[i]->ComputeHashTree();

	DiffCore   d;
	NodeHasher hasher;
	d.Diff<Node*, NodeHasher>(na, nb, a, b, hasher, apply);
}

struct CharHasher {
	uint32_t operator()(char c) const {
		return (uint32_t) c;
	}
};

XO_API std::string DiffTest(const char* a, const char* b, std::vector<int>& ops) {
	std::string r = a;

	auto patch = [&](PatchOp op, size_t pos, size_t len, const char* el) {
		if (op == PatchOp::Delete) {
			ops.push_back(-(int) len);
			r.erase(pos, len);
		} else if (op == PatchOp::Insert) {
			ops.push_back((int) len);
			r.insert(pos, el, len);
		}
	};

	DiffCore   d;
	CharHasher hasher;
	d.Diff<char, CharHasher>(strlen(a), strlen(b), a, b, hasher, patch);
	return r;
}

} // namespace vdom
} // namespace xo
