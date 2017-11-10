#pragma once
#include "../Defs.h"
#include "Observer.h"

namespace xo {
namespace rx {

class XO_API Control : public Observer {
public:
	xo::DomNode* Root = nullptr;

	// If root is not null, calls Bind(root)
	Control(xo::DomNode* root = nullptr);
	virtual ~Control();

	virtual void Render() = 0;
	virtual void RenderDiffable(std::string& content) {}

	// Implementation of Observer
	void ObservableTouched(Observable* target) override;

	void Bind(xo::DomNode* root);
	void SetDirty();
	bool IsDirty() const { return Dirty; }

	static void OnDocLifecycle(Event& ev);

private:
	std::thread::id BoundThread = std::thread::id(); // Thread on which UI is expected to run, including all DOM manipulation
	bool            Dirty       = true;              // Hide Dirty behind getter/setter so that we can put breakpoints on SetDirty, and maybe do other things at that moment.
};

} // namespace rx
} // namespace xo