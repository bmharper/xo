#include "pch.h"
#include "xoStyleResolve.h"
#include "xoRenderStack.h"

void xoStyleResolver::ResolveAndPush(xoRenderStack& stack, const xoDomNode* node)
{
	xoRenderStackEl& result = stack.StackPush();

	// 1. Inherited by default
	for (int i = 0; i < xoNumInheritedStyleCategories; i++)
		SetInherited(stack, node, xoInheritedStyleCategories[i]);

	// 2. Tag style
	Set(stack, node, stack.Doc->TagStyles[node->GetTag()]);

	// 3. Classes
	const podvec<xoStyleClassID>& classes = node->GetClasses();
	for (intp i = 0; i < classes.size(); i++)
		Set(stack, node, *stack.Doc->ClassStyles.GetByID(classes[i]));

	// 4. Node Styles
	Set(stack, node, node->GetStyle());
}

void xoStyleResolver::Set(xoRenderStack& stack, const xoDomEl* node, const xoStyleClass& klass)
{
	Set(stack, node, klass.Default);

	if (stack.Doc->UI.IsHovering(node->GetInternalID()))
		Set(stack, node, klass.Hover);

	if (stack.Doc->UI.IsFocused(node->GetInternalID()))
		Set(stack, node, klass.Focus);

	if (!klass.Hover.IsEmpty())
		stack.StackBack().HasHoverStyle = true;

	if (!klass.Focus.IsEmpty())
		stack.StackBack().HasFocusStyle = true;
}

void xoStyleResolver::Set(xoRenderStack& stack, const xoDomEl* node, const xoStyle& style)
{
	Set(stack, node, style.Attribs.size(), &style.Attribs[0]);
}

void xoStyleResolver::Set(xoRenderStack& stack, const xoDomEl* node, intp n, const xoStyleAttrib* vals)
{
	xoRenderStackEl& result = stack.StackBack();

	for (intp i = 0; i < n; i++)
	{
		if (vals[i].IsInherit())
			SetInherited(stack, node, vals[i].GetCategory());
		else
			result.Styles.Set(vals[i], result.Pool);
	}
}

void xoStyleResolver::SetInherited(xoRenderStack& stack, const xoDomEl* node, xoStyleCategories cat)
{
	xoRenderStackEl& result = stack.StackBack();
	intp stackSize = stack.StackSize();

	for (intp j = stackSize - 2; j >= 0; j--)
	{
		xoStyleAttrib attrib = stack.StackAt(j).Styles.Get(cat);
		if (!attrib.IsNull())
		{
			result.Styles.Set(attrib, result.Pool);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoStyleResolveOnceOff::xoStyleResolveOnceOff(const xoDomNode* node)
{
	RS = new xoRenderStack();
	Pool = new xoPool();
	RS->Initialize(node->GetDoc(), Pool);

	pvect<const xoDomNode*> chain;
	for (const xoDomNode* nodeWalk = node; nodeWalk != nullptr; nodeWalk = nodeWalk->GetParent())
		chain += nodeWalk;

	// The user of this class might want to be made aware if the node walk
	// fails before hitting the root.
	if (chain.back() != &node->GetDoc()->Root)
		XOTRACE_WARNING("Warning: xoStyleResolveOnceOff failed to walk back up to root\n");

	for (intp i = chain.size() - 1; i >= 0; i--)
		xoStyleResolver::ResolveAndPush(*RS, chain[i]);
}

xoStyleResolveOnceOff::~xoStyleResolveOnceOff()
{
	delete RS;
	delete Pool;
}
