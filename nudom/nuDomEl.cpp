#include "pch.h"
#include "nuDoc.h"
#include "nuDomEl.h"
#include "nuCloneHelpers.h"

nuDomEl::nuDomEl()
{
	Doc = NULL;
	AllEventMask = 0;
	InternalID = 0;
}

nuDomEl::~nuDomEl()
{
	delete_all( Children );
}

void nuDomEl::CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const
{
	c.InternalID = InternalID;
	c.Tag = Tag;
	Style.CloneFastInto( c.Style, pool );

	// copy classes
	nuClonePodvecPrepare( c.Classes, Classes, pool );
	memcpy( c.Classes.data, Classes.data, sizeof(Classes[0]) * Classes.size() );

	// alloc pointer list of chilren
	nuClonePvectPrepare( c.Children, Children, pool );
	
	// alloc children
	for ( int i = 0; i < Children.size(); i++ )
		c.Children[i] = pool->AllocT<nuDomEl>( true );
	
	// copy children
	for ( int i = 0; i < Children.size(); i++ )
		Children[i]->CloneFastInto( *c.Children[i], pool, cloneFlags );

	if ( !!(cloneFlags & nuCloneFlagEvents) )
		NUPANIC("clone events is TODO");
}

void nuDomEl::AddChild( nuDomEl* c )
{
	Children += c;
	Doc->ChildAdded( c );
}

void nuDomEl::RemoveChild( nuDomEl* c )
{
	if ( !c ) return;
	intp ix = Children.find( c );
	NUASSERT( ix != -1 );
	Children.erase(ix);
	Doc->ChildRemoved( c );
	delete c;
}

nuDomEl* nuDomEl::ChildByIndex( intp index )
{
	NUASSERT( (uintp) index < (uintp) Children.size() );
	return Children[index];
}

void nuDomEl::Discard()
{
	InternalID = 0;
	AllEventMask = 0;
	Style.Discard();
	Classes.hack( 0, 0, NULL );
	Children.hack( 0, 0, NULL );
	Handlers.hack( 0, 0, NULL );
}

void nuDomEl::AddClass( const char* klass )
{

}

void nuDomEl::RemoveClass( const char* klass )
{
}

void nuDomEl::AddHandler( nuEvents ev, nuEventHandlerF func, void* context )
{
	for ( intp i = 0; i < Handlers.size(); i++ )
	{
		if ( Handlers[i].Context == context && Handlers[i].Func == func )
		{
			Handlers[i].Mask |= ev;
			RecalcAllEventMask();
			return;
		}
	}
	auto& h = Handlers.add();
	h.Context = context;
	h.Func = func;
	h.Mask = ev;
	RecalcAllEventMask();
}

void nuDomEl::RecalcAllEventMask()
{
	uint32 m = 0;
	for ( intp i = 0; i < Handlers.size(); i++ )
		m |= Handlers[i].Mask;
	AllEventMask = m;
}


nuRenderDomEl::nuRenderDomEl( nuPool* pool )
{
	SetPool( pool );
}

nuRenderDomEl::~nuRenderDomEl()
{
	Discard();
}

void nuRenderDomEl::SetPool( nuPool* pool )
{
	Children.Pool = pool;
}

void nuRenderDomEl::Discard()
{
	InternalID = 0;
	Style.Discard();
	Children.clear();
}
