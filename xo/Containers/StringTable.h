#pragma once
#include "../Base/xoString.h"

namespace xo {

// This is append-only. It was built to hold resource strings that need an integer ID,
// such as background texture names, or style variable IDs.
class XO_API StringTable {
public:
	StringTable();
	~StringTable();

	const char* GetStr(int id) const; // Returns null if the id is invalid.
	int         GetID(const char* str) const;
	int         GetOrCreateID(const char* str);

	// This assumes that StringTable is "append-only", which it currently is.
	// Knowing this allows us to make the clone from Document to Render Document trivially fast.
	void CloneFrom_Incremental(const StringTable& src);

protected:
	xo::Pool                   Pool;
	ohash::map<StringRaw, int> NameToID;
	cheapvec<const char*>      IDToName;
};
} // namespace xo
