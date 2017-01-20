#include "pch.h"
#include "StyleResolve.h"
#include "RenderStack.h"

namespace xo {

void StyleResolver::ResolveAndPush(RenderStack& stack, const DomNode* node) {
	RenderStackEl& result = stack.StackPush();

	// 1. Inherited by default
	for (int i = 0; i < NumInheritedStyleCategories; i++)
		SetInherited(stack, node, InheritedStyleCategories[i]);

	// 2. Tag style
	Set(stack, node, stack.Doc->TagStyles[node->GetTag()]);

	// 3. Classes
	const cheapvec<StyleClassID>& classes = node->GetClasses();
	for (size_t i = 0; i < classes.size(); i++)
		Set(stack, node, *stack.Doc->ClassStyles.GetByID(classes[i]));

	// 4. Node Styles
	Set(stack, node, node->GetStyle());
}

void StyleResolver::Set(RenderStack& stack, const DomEl* node, const StyleClass& klass) {
	Set(stack, node, klass.Default);

	if (stack.Doc->UI.IsHovering(node->GetInternalID()))
		Set(stack, node, klass.Hover);

	if (stack.Doc->UI.IsFocused(node->GetInternalID()))
		Set(stack, node, klass.Focus);

	if (stack.Doc->UI.IsCaptured(node->GetInternalID()))
		Set(stack, node, klass.Capture);

	if (!klass.Hover.IsEmpty())
		stack.StackBack().HasHoverStyle = true;

	if (!klass.Focus.IsEmpty())
		stack.StackBack().HasFocusStyle = true;

	if (!klass.Capture.IsEmpty())
		stack.StackBack().HasCaptureStyle = true;
}

void StyleResolver::Set(RenderStack& stack, const DomEl* node, const Style& style) {
	Set(stack, node, style.Attribs.size(), &style.Attribs[0]);
}

void StyleResolver::Set(RenderStack& stack, const DomEl* node, size_t n, const StyleAttrib* vals) {
	RenderStackEl& result = stack.StackBack();

	for (size_t i = 0; i < n; i++) {
		if (vals[i].IsInherit())
			SetInherited(stack, node, vals[i].GetCategory());
		else
			SetFinal(result, vals[i]);
	}
}

void StyleResolver::SetInherited(RenderStack& stack, const DomEl* node, StyleCategories cat) {
	RenderStackEl& result    = stack.StackBack();
	size_t         stackSize = stack.StackSize();

	for (ssize_t j = stackSize - 2; j >= 0; j--) {
		StyleAttrib attrib = stack.StackAt(j).Styles.Get(cat);
		if (!attrib.IsNull()) {
			SetFinal(result, attrib);
			break;
		}
	}
}

void StyleResolver::SetFinal(RenderStackEl& result, StyleAttrib attrib) {
	// Force conflicting categories to pick a winner, by clobbering any existing
	// style with a NULL value for itself. Ideally we could erase a style from StyleSet.
	StyleCategories nullify = CatNULL;
	switch (attrib.Category) {
	case CatVCenter: nullify  = CatBaseline; break;
	case CatBaseline: nullify = CatVCenter; break;
	}
	if (nullify != CatNULL)
		result.Styles.EraseOrSetNull(nullify);

	// Set the new attribute
	result.Styles.Set(attrib, result.Pool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleResolveOnceOff::StyleResolveOnceOff(const DomNode* node) {
	RS   = new RenderStack();
	Pool = new xo::Pool();
	RS->Initialize(node->GetDoc(), Pool);

	cheapvec<const DomNode*> chain;
	for (const DomNode* nodeWalk = node; nodeWalk != nullptr; nodeWalk = nodeWalk->GetParent())
		chain += nodeWalk;

	XO_ANALYSIS_ASSUME(node != nullptr);

	// The user of this class might want to be made aware if the node walk
	// fails before hitting the root.
	if (chain.back() != &node->GetDoc()->Root)
		XOTRACE_WARNING("Warning: StyleResolveOnceOff failed to walk back up to root\n");

	for (size_t i = chain.size() - 1; i != -1; i--)
		StyleResolver::ResolveAndPush(*RS, chain[i]);
}

StyleResolveOnceOff::~StyleResolveOnceOff() {
	delete RS;
	delete Pool;
}
}
