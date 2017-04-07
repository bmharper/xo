#include "pch.h"
#include "StringTableGC.h"

namespace xo {

// This is used when the caller is parsing a big string, and he wants to
// use a substring from that big string, as a key inside a hash table.
// In that case, the string is not null terminated, so here we guarantee
// that the string is null terminated.
// A companion idea is to add more state to StringRaw, in particular
// make it cache it's hash code, and it's length. But that makes
// StringRaw much more complex.
class TruncatedTempString : public StringRaw {
public:
	char StaticBuf[64]; // A large buffer here has an unfortunate cost in MSVC debug builds, but aside from that, bigger is generally better here
	bool IsDynamic = false;

	// Assume that str[len] is addressable, if len != -1
	TruncatedTempString(const char* str, size_t len = -1) {
		if (len == -1 || str[len] == 0) {
			Z = const_cast<char*>(str);
		} else {
			// We need to make a copy
			if (len + 1 > sizeof(StaticBuf)) {
				IsDynamic = true;
				Z         = (char*) malloc(len + 1);
			} else {
				Z = StaticBuf;
			}
			memcpy(Z, str, len);
			Z[len] = 0;
		}
	}

	~TruncatedTempString() {
		if (IsDynamic)
			free(Z);
	}
};

StringTableGC::StringTableGC() {
	// Ensure that ID 0 is invalid
	IDToStr.push(nullptr);
	IsModified.push(false);
}

StringTableGC::~StringTableGC() {
}

const char* StringTableGC::GetStr(int id) const {
	if ((size_t) id >= (size_t) IDToStr.size())
		return nullptr;
	return IDToStr[id];
}

int StringTableGC::GetID(const char* str, size_t len) const {
	XO_ASSERT(str != nullptr);
	return StrToID.get(TruncatedTempString(str, len));
}

int StringTableGC::GetOrCreateID(const char* str, size_t len) {
	XO_ASSERT(str != nullptr);
	int                 id = 0;
	TruncatedTempString tstr(str, len);
	if (StrToID.get(tstr, id))
		return id;

	if (FreeIDList.size() != 0) {
		id = FreeIDList.rpop();
	} else {
		id = (int) IDToStr.size();
		IDToStr.push(nullptr);
		IsModified.push(true);
	}

	SetIDToStr(id, str, len, true);
	StrToID.insert(StringRaw::WrapConstAway(IDToStr[id]), id);
	return id;
}

void StringTableGC::SetIDToStr(int id, const char* str, size_t len, bool toggleModified) {
	if (len == -1)
		len = strlen(str);
	auto copy = (char*) Pool.Alloc(len + 1, false);
	memcpy(copy, str, len);
	copy[len]   = 0;
	IDToStr[id] = copy;
	if (toggleModified)
		IsModified[id] = true;
}

void StringTableGC::CloneFrom_Incremental(const StringTableGC& src) {
	bool repacked = RepackCount != src.RepackCount;
	if (repacked) {
		// src has been repacked since we last cloned, so to prevent us from also becoming
		// a memory leak, we choose to repack now too.
		RepackCount = src.RepackCount;
		IDToStr.clear_noalloc();
		Pool.FreeAll();
	}

	// Since this function is only used to clone elements for rendering,
	// and rendering doesn't need to go from String -> ID, we can avoid
	// cloning the hash table here.

	while (IDToStr.size() < src.IDToStr.size())
		IDToStr.push(nullptr);

	if (repacked) {
		// If a repack has taken place, then just copy everything
		for (size_t i = 1; i < src.IDToStr.size(); i++) {
			if (src.IDToStr[i])
				SetIDToStr((int) i, src.IDToStr[i], -1, false);
		}
	} else {
		for (size_t i = 1; i < src.IsModified.size(); i++) {
			if (src.IsModified[i])
				SetIDToStr((int) i, src.IDToStr[i], -1, false);
		}
	}
}

void StringTableGC::ResetModified() {
	IsModified.fill(false);
}

void StringTableGC::GCResetMark() {
	while (IsGCMarked.size() < IDToStr.size())
		IsGCMarked.push(false);
	IsGCMarked.fill(false);
}

void StringTableGC::GCSweep(bool forceRepack) {
	for (size_t i = 0; i < IsGCMarked.size(); i++) {
		if (!IsGCMarked[i] && IDToStr[i]) {
			WastedBytes += strlen(IDToStr[i]) + 1;
			StrToID.erase(StringRaw::WrapConstAway(IDToStr[i]));
			IDToStr[i] = nullptr;
		}
	}

	// I don't know what good numbers are here. These are just thumb suck, but probably OK.
	if (forceRepack || (WastedBytes > Pool.GetChunkSize() && WastedBytes > Pool.TotalAllocatedBytes() / 2))
		GCRepack();
}

void StringTableGC::GCRepack() {
	xo::Pool newPool;

	StrToID.clear_noalloc();
	FreeIDList.clear_noalloc();
	for (size_t i = 0; i < IDToStr.size(); i++) {
		if (IDToStr[i]) {
			auto len    = strlen(IDToStr[i]);
			auto newPtr = (char*) newPool.Alloc(len + 1, false);
			memcpy(newPtr, IDToStr[i], len);
			newPtr[len] = 0;
			IDToStr[i]  = newPtr;
			StrToID.insert(StringRaw::Wrap(newPtr), (int) i);
		} else if (i != 0) {
			FreeIDList.push((int) i);
		}
	}

	RepackCount++;
	WastedBytes = 0;
	std::swap(newPool, Pool);
}
}
