#include "pch.h"
#include "Registry.h"

namespace xo {
namespace rx {

void Registry::Register(const char* name, CreateFunc create) {
	Constructors.insert(name, create);
}

Control* Registry::Create(const char* name) {
	auto create = Constructors.get(TempString(name));
	if (create)
		return create();
	else
		return nullptr;
}

} // namespace rx
} // namespace xo
