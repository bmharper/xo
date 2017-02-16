#pragma once
#include "StringTable.h"

namespace xo {

class Doc;

// WHAT AM I THINKING USING A GC TABLE FOR VARIABLES!!
// Variables are not something that you can just prune. If the user
// sets a variable, then it's in there forever. Just because it's not
// referenced by the DOM, or by a class, doesn't mean you can delete it!

/* This holds the values of style variables, such as $dark-border = #333.
Every variable has a non-zero uint32 id, and it's value is stores in
origin string format. This is necessary for things such as:

	border: $bwidth #aaa

It's impossible for us to parse $bwidth upfront, so we need to delay
it until expansion time.

StringTable, which is used by this class, assumes that values are never
deleted. That will cause memory bloat if you don't recycle variable names.
*/
class XO_API VariableTable {
public:
	VariableTable(xo::Doc* doc);
	~VariableTable();

	void Set(const char* var, const char* value, size_t valueMaxLen = -1);

	// Returns null if not defined
	const char* Get(const char* var) const;

	void CloneFrom_Incremental(const VariableTable& src);
	void ResetModified();

protected:
	Doc*                 Doc;
	StringTable          IDTable;    // Store mapping to/from uint32 IDs of variable names
	cheapvec<xo::String> Values;     // Index is the ID, so entry zero is always an empty style
	cheapvec<bool>       IsModified; // Parallel to Values. Records whether a variable has been changed since last doc -> renderdoc sync. TODO: Change to proper bitmap
};
}
