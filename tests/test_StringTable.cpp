#include "pch.h"

TESTFUNC(StringTable)
{
	{
		xo::StringTableGC t, clone;

		for (int i = 1; i <= 10000; i++) {
			int id = t.GetOrCreateID(tsf::fmt("my string %v", i).c_str());
			TTASSERT(id == i);
		}

		clone.CloneFrom_Incremental(t);
		t.ResetModified();
		TTASSERT(strcmp(clone.GetStr(5), t.GetStr(5)) == 0);

		// delete 2/3 of our elements
		t.GCResetMark();
		for (int i = 1; i <= 10000; i++) {
			if (i % 3 == 0)
				t.GCMark(i);
		}
		t.GCSweep(true);
		clone.CloneFrom_Incremental(t);
		t.ResetModified();

		// ID zero is always null
		TTASSERT(t.GetStr(0) == nullptr);
		TTASSERT(clone.GetStr(0) == nullptr);

		// Verify values after GC sweep
		for (int i = 1; i <= 10000; i++) {
			if (i % 3 == 0) {
				TTASSERT(t.GetStr(i) == tsf::fmt("my string %v", i));
				TTASSERT(clone.GetStr(i) == tsf::fmt("my string %v", i));
			} else {
				TTASSERT(t.GetStr(i) == nullptr);
				TTASSERT(clone.GetStr(i) == nullptr);
			}
		}
	}
}