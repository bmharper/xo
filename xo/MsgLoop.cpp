#include "pch.h"
#include "Defs.h"
#include "DocGroup.h"
#include "MsgLoop.h"

namespace xo {

bool AnyDocsDirty() {
	for (size_t i = 0; i < Global()->Docs.size(); i++) {
		if (Global()->Docs[i]->IsDirty())
			return true;
	}
	return false;
}

}