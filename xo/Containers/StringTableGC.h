#pragma once
#include "Base/xoString.h"

namespace xo {

/* Garbage collected string table.
This string table is not append-only. It can clean up it's garbage.
This has the implication that IDs are recycled. We never lower the number
of ID slots. When running a garbage collection repack, we reallocate
memory for all our strings, and repack them tightly into that new space.
*/
class XO_API StringTableGC {
public:
	StringTableGC();
	~StringTableGC();

	const char* GetStr(int id) const; // Returns null if the id is invalid or zero (ie does not segfault).
	int         GetID(const char* str, size_t len = -1) const;
	int         GetOrCreateID(const char* str, size_t len = -1);

	void CloneFrom_Incremental(const StringTableGC& src);
	void ResetModified();

	void GCResetMark();
	void GCMark(int id) { IsGCMarked[id] = true; }
	void GCSweep(bool forceRepack = false);

protected:
	Pool                       Pool;
	ohash::map<StringRaw, int> StrToID;
	cheapvec<const char*>      IDToStr;
	cheapvec<bool>             IsGCMarked;
	cheapvec<bool>             IsModified; // Used by CloneFrom_Incremental
	cheapvec<int>              FreeIDList;
	size_t                     WastedBytes = 0; // Number of bytes in Pool that are garbage
	size_t                     RepackCount = 0;

	// Make a copy of str in Pool, and store it in IDToStr
	void SetIDToStr(int id, const char* str, size_t len, bool toggleModified);

	// Repack our strings into a new Pool.
	void GCRepack();
};
}
