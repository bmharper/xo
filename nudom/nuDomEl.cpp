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
	for ( intp i = 0; i < Children.size(); i++ )
		Doc->FreeChild( Children[i] );
	Children.clear();
}

// memory allocations come from the regular heap. This also happens to not be recursive.
void nuDomEl::CloneSlowInto( nuDomEl& c, uint cloneFlags ) const
{
	nuDoc* cDoc = c.GetDoc();
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	c.Text = Text;
	
	Style.CloneSlowInto( c.Style );
	c.Classes = Classes;

	// By the time we get here, all relevant DOM elements inside the destination document
	// have already been created. That is why we are not recursive here.
	c.Children.clear_noalloc();
	for ( intp i = 0; i < Children.size(); i++ )
		c.Children += cDoc->GetChildByInternalIDMutable( Children[i]->InternalID );

	if ( !!(cloneFlags & nuCloneFlagEvents) )
		NUPANIC("clone events is TODO");
}

// all memory allocations come from the pool. The also happens to be recursive.
void nuDomEl::CloneFastInto( nuDomEl& c, nuPool* pool, uint cloneFlags ) const
{
	nuDoc* cDoc = c.GetDoc();
	c.InternalID = InternalID;
	c.Tag = Tag;
	c.Version = Version;
	Style.CloneFastInto( c.Style, pool );

	NUASSERT(false); // Text?

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

	cDoc->ChildAddedFromDocumentClone( &c );
}

nuDomEl* nuDomEl::AddChild( nuTag tag )
{
	IncVersion();
	nuDomEl* c = Doc->AllocChild();
	c->Tag = tag;
	c->Doc = Doc;
	Children += c;
	Doc->ChildAdded( c );
	return c;
}

void nuDomEl::RemoveChild( nuDomEl* c )
{
	if ( !c ) return;
	IncVersion();
	intp ix = Children.find( c );
	NUASSERT( ix != -1 );
	Children.erase(ix);
	Doc->ChildRemoved( c );
	Doc->FreeChild( c );
}

void nuDomEl::RemoveAllChildren()
{
	IncVersion();
	for ( intp i = 0; i < Children.size(); i++ )
	{
		Doc->ChildRemoved( Children[i] );
		Doc->FreeChild( Children[i] );
	}
	Children.clear();
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

void nuDomEl::ForgetChildren()
{
	Children.clear_noalloc();
}

void nuDomEl::SetText( const char* txt )
{
	if ( Tag == nuTagText )
	{
		Text = txt;
	}
	else
	{
		if ( Children.size() != 1 || Children[0]->GetTag() != nuTagText )
		{
			RemoveAllChildren();
			AddChild( nuTagText );
		}
		Children[0]->Text = txt;
	}
}

const nuString&	nuDomEl::GetText() const
{
	if ( Tag == nuTagText )
	{
		return Text;
	}
	else if ( Children.size() == 1 && Children[0]->GetTag() == nuTagText )
	{
		return Children[0]->Text;
	}
	else
	{
		NUASSERT(Text.Length() == 0);
		return Text;
	}
}

bool nuDomEl::StyleParse( const char* t )
{
	IncVersion();
	return Style.Parse( t, Doc );
}

bool nuDomEl::StyleParsef( const char* t, ... )
{
	char buff[8192];
	va_list va;
	va_start( va, t );
	uint r = vsnprintf( buff, arraysize(buff), t, va );
	va_end( va );
	buff[arraysize(buff) - 1] = 0;
	if ( r < arraysize(buff) )
	{
		return StyleParse( buff );
	}
	else
	{
		nuString str = nuString(t);
		str.Z[50] = 0;
		nuParseFail( "Parse string is too long for StyleParsef: %s...", str.Z );
		NUASSERTDEBUG(false);
		return false;
	}
}

void nuDomEl::HackSetStyle( const nuStyle& style )
{
	IncVersion();
	Style = style;
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

void nuDomEl::AddHandler( nuEvents ev, nuEventHandlerF func, bool isLambda, void* context )
{
	for ( intp i = 0; i < Handlers.size(); i++ )
	{
		if ( Handlers[i].Context == context && Handlers[i].Func == func )
		{
			NUASSERT(isLambda == Handlers[i].IsLambda());
			Handlers[i].Mask |= ev;
			RecalcAllEventMask();
			return;
		}
	}
	auto& h = Handlers.add();
	h.Context = context;
	h.Func = func;
	h.Mask = ev;
	if ( isLambda )
		h.SetLambda();
	RecalcAllEventMask();
}

void nuDomEl::AddHandler( nuEvents ev, nuEventHandlerLambda lambda )
{
	nuEventHandlerLambda* copy = new nuEventHandlerLambda( lambda );
	AddHandler( ev, nuEventHandler_LambdaStaticFunc, true, copy );
}

void nuDomEl::AddHandler( nuEvents ev, nuEventHandlerF func, void* context )
{
	AddHandler( ev, func, false, context );
}

void nuDomEl::RecalcAllEventMask()
{
	uint32 m = 0;
	for ( intp i = 0; i < Handlers.size(); i++ )
		m |= Handlers[i].Mask;
	AllEventMask = m;
}

void nuDomEl::IncVersion()
{
	Version++;
	Doc->SetChildModified( InternalID );
}

