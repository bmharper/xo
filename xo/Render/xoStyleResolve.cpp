#include "pch.h"
#include "xoStyleResolve.h"
#include "xoRenderStack.h"

void xoStyleResolver::ResolveAndPush( xoRenderStack& stack, const xoDomNode* node )
{
	xoRenderStackEl& result = stack.StackPush();

	// 1. Inherited by default
	for ( int i = 0; i < xoNumInheritedStyleCategories; i++ )
		SetInherited( stack, node, xoInheritedStyleCategories[i] );

	// 2. Tag style
	Set( stack, node, stack.Doc->TagStyles[node->GetTag()] );

	// 3. Classes
	const podvec<xoStyleID>& classes = node->GetClasses();
	for ( intp i = 0; i < classes.size(); i++ )
		Set( stack, node, *stack.Doc->ClassStyles.GetByID( classes[i] ) );

	// 4. Node Styles
	Set( stack, node, node->GetStyle() );
}

void xoStyleResolver::Set( xoRenderStack& stack, const xoDomEl* node, const xoStyle& style )
{
	Set( stack, node, style.Attribs.size(), &style.Attribs[0] );
}

void xoStyleResolver::Set( xoRenderStack& stack, const xoDomEl* node, intp n, const xoStyleAttrib* vals )
{
	xoRenderStackEl& result = stack.StackBack();

	for ( intp i = 0; i < n; i++ )
	{
		if ( vals[i].IsInherit() )
			SetInherited( stack, node, vals[i].GetCategory() );
		else
			result.Styles.Set( vals[i], result.Pool );
	}
}

void xoStyleResolver::SetInherited( xoRenderStack& stack, const xoDomEl* node, xoStyleCategories cat )
{
	xoRenderStackEl& result = stack.StackBack();
	intp stackSize = stack.StackSize();

	for ( intp j = stackSize - 2; j >= 0; j-- )
	{
		xoStyleAttrib attrib = stack.StackAt(j).Styles.Get( cat );
		if ( !attrib.IsNull() )
		{
			result.Styles.Set( attrib, result.Pool );
			break;
		}
	}
}
