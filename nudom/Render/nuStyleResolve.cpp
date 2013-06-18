#include "pch.h"
#include "nuStyleResolve.h"

// Styles that are inherited by default
const nuStyleCategories nuStyleResolver::Inherited[nuStyleResolver::NumInherited] = {
	nuCatFontFamily,
};

void nuStyleResolver::Resolve( nuDoc* doc, nuRenderDomEl* root, nuPool* pool )
{
	Doc = doc;
	Pool = pool;
	ResolveNode( root, root );
}

void nuStyleResolver::ResolveNode( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode )
{
	const nuDomEl* node = Doc->GetChildByInternalID( rnode->InternalID );

	// 1. Inherited by default
	for ( int i = 0; i < NumInherited; i++ )
	{
		nuStyleAttrib attrib = parentRNode->ResolvedStyle.Get( Inherited[i] );
		if ( !attrib.IsNull() )
			rnode->ResolvedStyle.Set( attrib, Pool );
	}

	// 2. Classes
	const podvec<nuStyleID>& classes = node->GetClasses();
	for ( intp i = 0; i < classes.size(); i++ )
		Set( parentRNode, rnode, *Doc->ClassStyles.GetByID( classes[i] ) );

	// 3. Node Styles
	Set( parentRNode, rnode, node->GetStyle() );
	
}

void nuStyleResolver::Set( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode, const nuStyle& style )
{
	Set( parentRNode, rnode, (int) style.Attribs.size(), &style.Attribs[0] );
}

void nuStyleResolver::Set( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode, int n, const nuStyleAttrib* vals )
{
	for ( int i = 0; i < n; i++ )
	{
		if ( vals[i].IsInherit() )
		{
			nuStyleAttrib inherited = parentRNode->ResolvedStyle.Get( vals[i].GetCategory() );
			if ( !inherited.IsNull() )
				rnode->ResolvedStyle.Set( inherited, Pool );
		}
		else
		{
			rnode->ResolvedStyle.Set( vals[i], Pool );
		}
	}
}
