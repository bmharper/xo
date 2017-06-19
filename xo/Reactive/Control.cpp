#include "pch.h"
#include "Control.h"
#include "../Dom/DomNode.h"

namespace xo {
namespace rx {

Control::Control(xo::DomNode* root) {
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
}

void Control::OnDocProcess(Event& ev) {
	Control* self = (Control*) ev.Context;
	if (self->Dirty) {
		self->Render();
		self->Dirty = false;
	}
}

} // namespace rx
} // namespace xo
