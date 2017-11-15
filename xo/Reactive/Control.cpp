#include "pch.h"
#include "Control.h"
#include "../Dom/DomNode.h"
#include "../Doc.h"
#include "../VirtualDom/Diff.h"
#include "../Parse/DocParser.h"

namespace xo {
namespace rx {

Control::Control() {
	BoundThread = std::this_thread::get_id();
	PrevTree    = vdom::Node();
}

Control::~Control() {
}

void Control::RenderRoot() {
	vdom::VDomToRealDom vd(xo::Global()->ControlRegistry);

	std::string dom;
	Render(dom);

	DocParser  parser;
	Pool       treePool;
	vdom::Node tree = {};

	auto err = parser.Parse(dom.c_str(), &tree, &treePool);

	// Give Node a name, so that it knows it's a node. We don't strictly need to copy "control-root" into the mem pool,
	// but the inconsistency of having all other strings in the pool scares me. If we don't do this, then 'tree' doesn't
	// know that it's a Node, and functions don't recurse down into it (ie functions such as ComputeHashTree).
	tree.Name = treePool.CopyStr("control-root");
	tree.ComputeHashTree(vdom::Node::HashMode::NameOnly);

	ApplyDiff_R(vd, Owner, &PrevTree, &tree);

	std::swap(PrevTreePool, treePool);
	std::swap(PrevTree, tree);

	// just get something alive...
	Owner->OnMouseDown(_OnAny, this);
}

// va can be null, if this is a new element.
// vb is always defined.
String Control::ApplyDiff_R(vdom::VDomToRealDom& vd, DomEl* ownerEl, vdom::Node* va, vdom::Node* vb) {
	if (ownerEl->IsText()) {
		// If this is a text element, then we have very little to do
		XO_ASSERT(vb->IsText());
		ownerEl->SetText(vb->Val);
		return "";
	}

	DomNode* owner = (DomNode*) ownerEl;

	// If any attributes have changed, then rebuild all attribs
	if (!va || !va->EqualAttribs(*vb)) {
		owner->RemoveAllClasses();
		owner->ClearStyle();
		auto err = DocParser::SetAttributes(vb, owner);
		if (err != "")
			return err;
	}

	// Perform diff on children
	size_t nchild_a = va ? va->NChild : 0;
	size_t nchild_b = vb->NChild;

	// Keep track of the elements of 'a', as we delete and insert, so that when we walk further down the tree,
	// we can compare the subchildren of 'b' with their counterparts in 'a'. Not every node in 'b' will have a counterpart
	// in shifted_a.
	cheapvec<vdom::Node*> shifted_a;
	for (size_t i = 0; i < nchild_a; i++)
		shifted_a.push(va->Children[i]);

	String err;
	vdom::Diff(vdom::Node::HashMode::NameOnly, nchild_a, va ? va->Children : nullptr, nchild_b, vb->Children, [&](vdom::PatchOp op, size_t pos, size_t len, vdom::Node* const* first) {
		if (err != "")
			return;
		switch (op) {
		case vdom::PatchOp::Delete:
			owner->DeleteChildren(pos, len);
			shifted_a.erase(pos, pos + len);
			break;
		case vdom::PatchOp::Insert:
			size_t orgSize = shifted_a.size();
			for (size_t i = 0; i < len; i++) {
				shifted_a.push(nullptr);
				err = vd.CreateChild(owner, pos + i, first[i]);
				if (err != "")
					break;
			}
			// create a range of null entries in shifted_a where these new children were inserted
			memmove(&shifted_a[pos + len], &shifted_a[pos], (orgSize - pos) * sizeof(shifted_a[0]));
			for (size_t i = 0; i < len; i++)
				shifted_a[pos + i] = nullptr;
			break;
		}
	});
	if (err != "")
		return err;

	XO_ASSERT(shifted_a.size() == vb->NChild);

	// Now that array vb->Children is aligned to owner->Children, we can recursively walk down.
	for (size_t i = 0; i < nchild_b; i++) {
		err = ApplyDiff_R(vd, owner->ChildByIndex(i), shifted_a[i], vb->Children[i]);
		if (err != "")
			return err;
	}

	return err;
}

void Control::ObservableTouched(Observable* target) {
	SetDirty();
}

void Control::Bind(xo::DomNode* owner, bool isRoot) {
	Owner = owner;
	if (isRoot)
		Owner->OnDocLifecycle(_OnDocLifecycle, this);
	Owner->OnDestroy(_OnDestroy, this);
}

void Control::SetDirty() {
	// First approximation: Set all ancestors to dirty, so that top-level root control re-renders everything. Just to get something on the screen.
	for (auto p = Parent; p; p = p->Parent) {
		p->Dirty = true;
	}
	Dirty = true;

	if (std::this_thread::get_id() != BoundThread) {
		// Make sure that the xo message loop wakes up to re-render us.
		// This message was sent from another thread (something doing background processing), so
		// the main xo message loop won't necessarily have any reason to invoke the DocLifecycle mechanism.
		// What happens after this?
		// Basically, we need to somehow inject a message into the OS window message queue, so that
		// the main thread wakes up and performs a re-render. On Windows, we use a custom WM_USER message.
		Owner->GetDoc()->TouchedByOtherThread();
	}
}

void Control::_OnDocLifecycle(Event& ev) {
	if (ev.DocLifecycle != DocLifecycleEvents::DispatchEnd)
		return;
	Control* self = (Control*) ev.Context;
	if (self->Dirty) {
		self->RenderRoot();
		self->Dirty = false;
	}
}

void Control::_OnDestroy(Event& ev) {
	Control* self = (Control*) ev.Context;
	if (self->IsRoot()) {
		delete self;
	} else {
		// We don't know yet what to do with this Control. It's possible that it's going to get reused during the next render.
		self->Dead = true;
	}
}

void Control::_OnAny(Event& ev) {
	Control* self = (Control*) ev.Context;
	self->OnEvent("mdown");
}

} // namespace rx
} // namespace xo
