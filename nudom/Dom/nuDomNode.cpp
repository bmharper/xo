#include "pch.h"
#include "nuDoc.h"
#include "nuDomNode.h"

nuDomNode::nuDomNode( nuDoc* doc, nuTag tag ) : nuDomEl(doc, tag)
{
	AllEventMask = 0;
}

nuDomNode::~nuDomNode()
{
	for ( intp i = 0; i < Children.size(); i++ )
		Doc->FreeChild( Children[i] );
	Children.clear();
}

void nuDomNode::SetText( const char* txt )
{
	if ( Children.size() != 1 || Children[0]->GetTag() != nuTagText )
	{
		RemoveAllChildren();
		AddChild( nuTagText );
	}
	Children[0]->SetText( txt );
}

const char* nuDomNode::GetText() const
{
	if ( Children.size() == 1 && Children[0]->GetTag() == nuTagText )
	{
		return Children[0]->GetText();
	}
	else
	{
		return "";
	}
}

void nuDomNode::CloneSlowInto( nuDomEl& c, uint cloneFlags ) const
{
	CloneSlowIntoBase( c, cloneFlags );
	nuDomNode& cnode = static_cast<nuDomNode&>(c);
	nuDoc* cDoc = c.GetDoc();
	
	Style.CloneSlowInto( cnode.Style );
	cnode.Classes = Classes;

	// By the time we get here, all relevant DOM elements inside the destination document
	// have already been created. That is why we are not recursive here.
	cnode.Children.clear_noalloc();
	for ( intp i = 0; i < Children.size(); i++ )
		cnode.Children += cDoc->GetChildByInternalIDMutable( Children[i]->GetInternalID() );

	if ( !!(cloneFlags & nuCloneFlagEvents) )
		NUPANIC("clone events is TODO");
}

void nuDomNode::ForgetChildren()
{
	Children.clear_noalloc();
}

nuDomEl* nuDomNode::AddChild( nuTag tag )
{
	IncVersion();
	nuDomEl* c = Doc->AllocChild( tag );
	Children += c;
	Doc->ChildAdded( c );
	return c;
}

nuDomNode* nuDomNode::AddNode( nuTag tag )
{
	AbcAssert( tag != nuTagText );
	return static_cast<nuDomNode*>(AddChild(tag));
}

nuDomText* nuDomNode::AddText( const char* txt )
{
	nuDomText* el = static_cast<nuDomText*>(AddChild(nuTagText));
	el->SetText( txt );
	return el;
}

void nuDomNode::RemoveChild( nuDomEl* c )
{
	if ( !c ) return;
	IncVersion();
	intp ix = Children.find( c );
	NUASSERT( ix != -1 );
	Children.erase(ix);
	Doc->ChildRemoved( c );
	Doc->FreeChild( c );
}

void nuDomNode::RemoveAllChildren()
{
	IncVersion();
	for ( intp i = 0; i < Children.size(); i++ )
	{
		Doc->ChildRemoved( Children[i] );
		Doc->FreeChild( Children[i] );
	}
	Children.clear();
}

nuDomEl* nuDomNode::ChildByIndex( intp index )
{
	NUASSERT( (uintp) index < (uintp) Children.size() );
	return Children[index];
}

const nuDomEl* nuDomNode::ChildByIndex( intp index ) const
{
	NUASSERT( (uintp) index < (uintp) Children.size() );
	return Children[index];
}

void nuDomNode::Discard()
{
	InternalID = 0;
	AllEventMask = 0;
	Version = 0;
	Style.Discard();
	Classes.hack( 0, 0, NULL );
	Children.hack( 0, 0, NULL );
	Handlers.hack( 0, 0, NULL );
}

bool nuDomNode::StyleParse( const char* t )
{
	IncVersion();
	return Style.Parse( t, Doc );
}

bool nuDomNode::StyleParsef( const char* t, ... )
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

void nuDomNode::HackSetStyle( const nuStyle& style )
{
	IncVersion();
	Style = style;
}

void nuDomNode::AddClass( const char* klass )
{
	IncVersion();
	nuStyleID id = Doc->ClassStyles.GetStyleID( klass );
	if ( Classes.find( id ) == -1 )
		Classes += id;
}

void nuDomNode::RemoveClass( const char* klass )
{
	IncVersion();
	nuStyleID id = Doc->ClassStyles.GetStyleID( klass );
	intp index = Classes.find( id );
	if ( index != -1 )
		Classes.erase( index );
}

void nuDomNode::AddHandler( nuEvents ev, nuEventHandlerF func, bool isLambda, void* context )
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

void nuDomNode::AddHandler( nuEvents ev, nuEventHandlerLambda lambda )
{
	nuEventHandlerLambda* copy = new nuEventHandlerLambda( lambda );
	AddHandler( ev, nuEventHandler_LambdaStaticFunc, true, copy );
}

void nuDomNode::AddHandler( nuEvents ev, nuEventHandlerF func, void* context )
{
	AddHandler( ev, func, false, context );
}

void nuDomNode::RecalcAllEventMask()
{
	uint32 m = 0;
	for ( intp i = 0; i < Handlers.size(); i++ )
		m |= Handlers[i].Mask;
	AllEventMask = m;
}
