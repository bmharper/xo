#include "pch.h"
#include "Control.h"
#include "../Dom/DomNode.h"
#include "../Doc.h"

namespace xo {
namespace rx {

Control::Control(xo::DomNode* root) {
	BoundThread = std::this_thread::get_id();
	if (root)
		Bind(root);
}

// It's vital to have a virtual destructor, so that we can do "delete this" from OnDestroy
Control::~Control() {
}

void Control::ObservableTouched(Observable* target) {
	SetDirty();
}

void Control::Bind(xo::DomNode* root) {
	Root = root;
	Root->OnDocLifecycle(OnDocLifecycle, this);
	Root->OnDestroy([this] {
		delete this;
	});
}

void Control::SetDirty() {
	Dirty = true;
	if (std::this_thread::get_id() != BoundThread) {
		// Make sure that the xo message loop wakes up to re-render us.
		// This message was sent from another thread (something doing background processing), so
		// the main xo message loop won't necessarily have any reason to invoke the DocLifecycle mechanism.
		// What happens after this?
		// Basically, we need to somehow inject a message into the OS window message queue, so that
		// the main thread wakes up and performs a re-render. On Windows, we use a custom WM_USER message.
		Root->GetDoc()->TouchedByOtherThread();
	}
}

void Control::OnDocLifecycle(Event& ev) {
	if (ev.DocLifecycle != DocLifecycleEvents::DispatchEnd)
		return;
	Control* self = (Control*) ev.Context;
	if (self->Dirty) {
		self->Render();
		self->Dirty = false;
	}
}

} // namespace rx
} // namespace xo
