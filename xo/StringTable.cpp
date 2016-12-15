#include "pch.h"
#include "StringTable.h"

namespace xo {

StringTable::StringTable() {
	IdToName += "";
	NameToId.insert(IdToName[0], 0);
}

StringTable::~StringTable() {
}

const char* StringTable::GetStr(int id) const {
	if ((uint32_t) id >= (uint32_t) IdToName.size())
		return "";
	return IdToName[id];
}

int StringTable::GetId(const char* str) {
	XO_ASSERT(str != nullptr);
	TempString s(str);
	int        v = 0;
	if (NameToId.get(s, v))
		return v;
	size_t len  = s.Length();
	char*  copy = (char*) Pool.Alloc(len + 1, false);
	memcpy(copy, str, len);
	copy[len] = 0;
	IdToName += copy;
	NameToId.insert(s, (int) IdToName.size() - 1);
	return (int) IdToName.size() - 1;
}

void StringTable::CloneFrom_Incremental(const StringTable& src) {
	for (size_t i = IdToName.size(); i < src.IdToName.size(); i++) {
		int id = GetId(src.IdToName[i]);
		XO_ASSERT(id == i);
	}
}
}
