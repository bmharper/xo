#include "pch.h"
#include "Button.h"
#include "../Reactive/Registry.h"

namespace xo {
namespace controls {
void RegisterAllBuiltin(rx::Registry* registry) {
	registry->Register("button", ButtonRx::Create);
}
} // namespace controls
} // namespace xo