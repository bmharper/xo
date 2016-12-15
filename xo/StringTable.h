#pragma once
#include "Base/xoString.h"

namespace xo {

class XO_API StringTable {
public:
	StringTable();
	~StringTable();

	const char* GetStr(int id) const; // Returns an empty string if the id is invalid or zero.
	int         GetId(const char* str);

	// This assumes that StringTable is "append-only", which it currently is.
	// Knowing this allows us to make the clone from Document to Render Document trivially fast.
	void CloneFrom_Incremental(const StringTable& src);

protected:
	Pool                    Pool;
	ohash::map<String, int> NameToId; // This could be improved dramatically, by avoiding the heap alloc for every item
	cheapvec<const char*>   IdToName;
};
}
