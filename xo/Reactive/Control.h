#pragma once
#include "../Defs.h"
#include "Observer.h"
#include "../VirtualDom/VirtualDom.h"

namespace xo {
namespace rx {

class XO_API Control : public Observer {
public:
	Control*     Parent = nullptr; // Control above us in the hierarchy. Root controls have a null Parent.
	xo::DomNode* Owner  = nullptr; // DOM node that we are associated with

	// Create a new root control that lives inside 'owner'
	template <typename T>
	static T* CreateRoot(xo::DomNode* owner) {
		T* t = new T();
		t->Bind(owner, true);
		return t;
	}

	Control();
	virtual ~Control(); // It's vital to have a virtual destructor, so that we can do "delete this" from OnDestroy

	// Control overridables
	virtual void Render(std::string& dom) {}
	virtual void OnEvent(const char* evname) {}

	// Implementation of Observer
	void ObservableTouched(Observable* target) override;

	bool IsRoot() const { return Parent == nullptr; }

	void Bind(xo::DomNode* owner, bool isRoot);
	void SetDirty();
	bool IsDirty() const { return Dirty; }

private:
	std::thread::id BoundThread = std::thread::id(); // Thread on which UI is expected to run, including all DOM manipulation
	bool            Dirty       = true;              // Hide Dirty behind getter/setter so that we can put breakpoints on SetDirty, and maybe do other things at that moment.
	bool            Dead        = false;             // If true, then our owning control has been destroyed. Used to track liveness of children.

	// State maintained only by root nodes
	Pool       PrevTreePool;
	vdom::Node PrevTree;

	void          RenderRoot();
	static String ApplyDiff_R(vdom::VDomToRealDom& vd, DomEl* ownerEl, vdom::Node* va, vdom::Node* vb);
	//static String ApplyDiff_R(vdom::VDomToRealDom& vd, DomNode* owner, size_t na, vdom::Node** a, size_t nb, vdom::Node** b);

	static void _OnDocLifecycle(Event& ev);
	static void _OnDestroy(Event& ev);
	static void _OnAny(Event& ev);
};

} // namespace rx
} // namespace xo