#include "pch.h"
#include "nuStyleResolve.h"
#include "nuRenderStack.h"

void nuStyleResolver::ResolveAndPush( nuRenderStack& stack, const nuDomNode* node )
{
	nuRenderStackEl& result = stack.StackPush();

	// 1. Inherited by default
	for ( int i = 0; i < nuNumInheritedStyleCategories; i++ )
		SetInherited( stack, node, nuInheritedStyleCategories[i] );

	// 2. Tag style
	Set( stack, node, stack.Doc->TagStyles[node->GetTag()] );

	// 3. Classes
	const podvec<nuStyleID>& classes = node->GetClasses();
	for ( intp i = 0; i < classes.size(); i++ )
		Set( stack, node, *stack.Doc->ClassStyles.GetByID( classes[i] ) );

	// 4. Node Styles
	Set( stack, node, node->GetStyle() );
}

void nuStyleResolver::Set( nuRenderStack& stack, const nuDomEl* node, const nuStyle& style )
{
	Set( stack, node, style.Attribs.size(), &style.Attribs[0] );
}

void nuStyleResolver::Set( nuRenderStack& stack, const nuDomEl* node, intp n, const nuStyleAttrib* vals )
{
	nuRenderStackEl& result = stack.StackBack();

	for ( intp i = 0; i < n; i++ )
	{
		if ( vals[i].IsInherit() )
			SetInherited( stack, node, vals[i].GetCategory() );
		else
			result.Styles.Set( vals[i], result.Pool );
	}
}

void nuStyleResolver::SetInherited( nuRenderStack& stack, const nuDomEl* node, nuStyleCategories cat )
{
	nuRenderStackEl& result = stack.StackBack();
	intp stackSize = stack.StackSize();

	for ( intp j = stackSize - 2; j >= 0; j-- )
	{
		nuStyleAttrib attrib = stack.StackAt(j).Styles.Get( cat );
		if ( !attrib.IsNull() )
		{
			result.Styles.Set( attrib, result.Pool );
			break;
		}
	}
}
