#pragma once

#include "xoString.h"

class XOAPI xoStringTable
{
public:
	xoStringTable();
	~xoStringTable();

	const char*			GetStr(int id) const;			// Returns an empty string if the id is invalid or zero.
	int					GetId(const char* str);

	// This assumes that xoStringTable is "append-only", which it currently is.
	// Knowing this allows us to make the clone from Document to Render Document trivially fast.
	void				CloneFrom_Incremental(const xoStringTable& src);

protected:
	xoPool						Pool;
	fhashmap<xoString, int>		NameToId;			// This could be improved dramatically, by avoiding the heap alloc for every item
	pvect<const char*>			IdToName;
};