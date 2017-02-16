#include "pch.h"
#include "StringTable.h"

namespace xo {

StringTable::StringTable() {
	IDToName.push(nullptr);
	NameToID.insert(StringRaw::WrapConstAway(IDToName[0]), 0);
}

StringTable::~StringTable() {
}

const char* StringTable::GetStr(int id) const {
	return IDToName[id];
}

int StringTable::GetID(const char* str) const {
	XO_ASSERT(str != nullptr);
	int v = 0;
	NameToID.get(StringRaw::WrapConstAway(str), v);
	return v;
}

int StringTable::GetOrCreateID(const char* str) {
	XO_ASSERT(str != nullptr);
	int v = 0;
	if (NameToID.get(StringRaw::WrapConstAway(str), v))
		return v;
	size_t len  = strlen(str);
	char*  copy = (char*) Pool.Alloc(len + 1, false);
	memcpy(copy, str, len);
	copy[len] = 0;
	IDToName += copy;
	NameToID.insert(StringRaw::WrapConstAway(copy), (int) IDToName.size() - 1);
	return (int) IDToName.size() - 1;
}

void StringTable::CloneFrom_Incremental(const StringTable& src) {
	for (size_t i = IDToName.size(); i < src.IDToName.size(); i++) {
		int id = GetOrCreateID(src.IDToName[i]);
		XO_ASSERT(id == i);
	}
}
}
