#include "pch.h"
#include "nuDoc.h"
#include "nuDomEl.h"
#include "nuCloneHelpers.h"

nuDomEl::nuDomEl()
{
	Doc = NULL;
	AllEventMask = 0;
	InternalID = 0;
	Version = 0;
}

nuDomEl::~nuDomEl()
{
	delete_all( Children );
}

void nuDomEl::CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const
{
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	Style.CloneFastInto( c.Style, pool );

	// copy classes
	nuClonePodvecWithMemCopy( c.Classes, Classes, pool );

	// alloc list of pointers to children
	nuClonePvectPrepare( c.Children, Children, pool );
	
	// alloc children
	for ( int i = 0; i < Children.size(); i++ )
		c.Children[i] = pool->AllocT<nuDomEl>( true );
	
	// copy children
	for ( int i = 0; i < Children.size(); i++ )
	{
		c.Children[i]->Doc = c.Doc;
		Children[i]->CloneFastInto( *c.Children[i], pool, cloneFlags );
	}

	if ( !!(cloneFlags & nuCloneFlagEvents) )
		NUPANIC("clone events is TODO");

	c.GetDoc()->ChildAddedFromDocumentClone( &c );
}

nuDomEl* nuDomEl::AddChild( nuTag tag )
{
	IncVersion();
	nuDomEl* c = new nuDomEl();
	c->Tag = tag;
	c->Doc = Doc;
	Children += c;
	Doc->ChildAdded( c );
	return c;
}

void nuDomEl::RemoveChild( nuDomEl* c )
{
	IncVersion();
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
	Version = 0;
	Style.Discard();
	Classes.hack( 0, 0, NULL );
	Children.hack( 0, 0, NULL );
	Handlers.hack( 0, 0, NULL );
}

bool nuDomEl::StyleParsef( const char* t, ... )
{
	char buff[8192] = "";
	va_list va;
	va_start( va, t );
	uint r = vsnprintf( buff, arraysize(buff), t, va );
	va_end( va );
	if ( r < arraysize(buff) )
	{
		return StyleParse( buff );
	}
	else
	{
		NUASSERTDEBUG(false);
		return false;
	}
}

void nuDomEl::AddClass( const char* klass )
{
	IncVersion();
	nuStyleID id = Doc->ClassStyles.GetStyleID( klass );
	if ( Classes.find( id ) == -1 )
		Classes += id;
}

void nuDomEl::RemoveClass( const char* klass )
{
	IncVersion();
	nuStyleID id = Doc->ClassStyles.GetStyleID( klass );
	intp index = Classes.find( id );
	if ( index != -1 )
		Classes.erase( index );
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


