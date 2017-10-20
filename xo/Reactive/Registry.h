#pragma once

namespace xo {
namespace rx {

class Control;

// Registry of reactive controls
// Before using a control, you must register it with this registry. This is necessary
// so that the virtual dom to real dom converter can recognize your controls inside
// the virtual dom.
class Registry {
public:
	typedef Control* (*CreateFunc)();

	void     Register(const char* name, CreateFunc create);
	Control* Create(const char* name);

private:
	ohash::map<String, CreateFunc> Constructors;
};

} // namespace rx
} // namespace xo