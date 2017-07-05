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
	Root->OnDocProcess(OnDocProcess, this);
	Root->OnDestroy([this] {
		delete this;
	});
}

void Control::SetDirty() {
	Dirty = true;
	if (std::this_thread::get_id() != BoundThread) {
		// Make sure that the xo message loop wakes up to re-render us.
		// This message was sent from another thread (something doing background processing), so
		// the main xo message loop won't necessarily have any reason to invoke the DocProcess mechanism.
		// And now? Need to get hold of our parent doc, and somehow inject a message into it's 
		// OS window message queue, so that it wakes up. On Windows, I'm thinking of using a custom
		// WM_USER message.
		Root->GetDoc()->TouchedByOtherThread();
	}
}

void Control::OnDocProcess(Event& ev) {
	if (ev.DocProcess != DocProcessEvents::DispatchEnd)
		return;
	Control* self = (Control*) ev.Context;
	if (self->Dirty) {
		self->Render();
		self->Dirty = false;
	}
}

} // namespace rx
} // namespace xo
