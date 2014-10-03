#include "pch.h"
#include "xoDoc.h"
#include "xoDocGroup.h"
#include "Layout/xoLayout.h"
#include "Layout/xoLayout2.h"
#include "Render/xoRenderer.h"
#include "Text/xoFontStore.h"
#include "xoCloneHelpers.h"
#include "xoStyle.h"

xoDoc::xoDoc()
	: Root( this, xoTagDiv, xoInternalIDNull ), UI( this )
{
	IsReadOnly = false;
	Version = 0;
	ClassStyles.AddDummyStyleZero();
	Root.SetDoc( this );
	Root.SetDocRoot();
	ResetInternalIDs();
	InitializeDefaultTagStyles();
}

xoDoc::~xoDoc()
{
	// TODO: Ensure that all of your events in the process-wide event queue have been dealt with,
	// because the event processor is going to try to access this doc.
	Reset();
}

void xoDoc::IncVersion()
{
	Version++;
}

void xoDoc::ResetModifiedBitmap()
{
	if ( ChildIsModified.Size() > 0 )
		ChildIsModified.Fill( 0, ChildIsModified.Size() - 1, false );
}

void xoDoc::MakeFreeIDsUsable()
{
	UsableIDs += FreeIDs;
	FreeIDs.clear();
}

/*
void xoDoc::CloneFastInto( xoDoc& c, uint cloneFlags, xoRenderStats& stats ) const
{
	// this code path died...
	XOASSERT(false);

	//c.Reset();

	if ( c.ChildByInternalID.size() != ChildByInternalID.size() )
		c.ChildByInternalID.resize( ChildByInternalID.size() );
	c.ChildByInternalID.nullfill();
	Root.CloneFastInto( c.Root, &c.Pool, cloneFlags );
	
	ClassStyles.CloneFastInto( c.ClassStyles, &c.Pool );
	xoCloneStaticArrayWithCloneFastInto( c.TagStyles, TagStyles, &c.Pool );
}
*/

// This clones only the objects that are marked as modified.
void xoDoc::CloneSlowInto( xoDoc& c, uint cloneFlags, xoRenderStats& stats ) const
{
	c.IsReadOnly = true;

	// Make sure the destination is large enough to hold all of our children
	while ( c.ChildByInternalID.size() < ChildByInternalID.size() )
		c.ChildByInternalID += nullptr;

	// Although it would be trivial to parallelize the following two passes, I think it is unlikely to be worth it,
	// since I suspect these passes will be bandwidth limited.

	// Pass 1: Ensure that all objects that are present in the source document have a valid pointer in the target document
	for ( int i = 0; i < ChildIsModified.Size(); i++ )
	{
		if ( ChildIsModified.Get(i) )
		{
			stats.Clone_NumEls++;
			const xoDomEl* src = GetChildByInternalID( i );
			xoDomEl* dst = c.GetChildByInternalIDMutable( i );
			if ( src && !dst )
			{
				// create in destination
				c.ChildByInternalID[i] = c.AllocChild( src->GetTag(), src->GetParentID() );
			}
			else if ( !src && dst )
			{
				// destroy destination. Make it forget its children, because this loop takes care of all elements.
				dst->ForgetChildren();
				c.FreeChild( dst );
				c.ChildByInternalID[i] = nullptr;
			}
		}
	}
	
	// Pass 2: Clone the contents of all our modified objects into our target
	for ( int i = 0; i < ChildIsModified.Size(); i++ )
	{
		if ( ChildIsModified.Get(i) )
		{
			const xoDomEl* src = GetChildByInternalID( i );
			xoDomEl* dst = c.GetChildByInternalIDMutable( i );
			if ( src )
				src->CloneSlowInto( *dst, cloneFlags );
		}
	}

	ClassStyles.CloneSlowInto( c.ClassStyles );
	xoCloneStaticArrayWithCloneSlowInto( c.TagStyles, TagStyles );

	c.Strings.CloneFrom_Incremental( Strings );

	c.Version = Version;

	UI.CloneSlowInto( c.UI );
}

bool xoDoc::ClassParse( const char* klass, const char* style )
{
	const char* colon = strchr( klass, ':' );
	xoString tmpKlass;
	xoString pseudo;
	if ( colon != nullptr )
	{
		tmpKlass = klass;
		tmpKlass.Z[colon - klass] = 0;
		pseudo = colon + 1;
		klass = tmpKlass.Z;
	}
	xoStyleClass* s = ClassStyles.GetOrCreate( klass );
	xoStyle* subset = nullptr;
	if ( pseudo.Length() == 0 )		{ subset = &s->Default; }
	else if ( pseudo == "hover" )	{ subset = &s->Hover; }
	else if ( pseudo == "focus" )	{ subset = &s->Focus; }
	else
	{
		return false;
	}
	return subset->Parse( style, this );
}

xoDomEl* xoDoc::AllocChild( xoTag tag, xoInternalID parentID )
{
	XOASSERT(tag != xoTagNULL);

	// we may want to use a more specialized heap in future, so we keep this allocation path strict
	if ( tag == xoTagText )
		return new xoDomText( this, tag, parentID );
	else
		return new xoDomNode( this, tag, parentID );
}

void xoDoc::FreeChild( const xoDomEl* el )
{
	// we may want to use a more specialized heap in future, so we keep this allocation path strict
	delete el;
}

xoString xoDoc::Parse( const char* src )
{
	return Root.Parse( src );
}

void xoDoc::ChildAdded( xoDomEl* el )
{
	XOASSERT(el->GetDoc() == this);
	XOASSERT(el->GetInternalID() == 0);
	if ( UsableIDs.size() != 0 )
	{
		el->SetInternalID( UsableIDs.rpop() );
		ChildByInternalID[el->GetInternalID()] = el;
	}
	else
	{
		el->SetInternalID( (xoInternalID) ChildByInternalID.size() );
		ChildByInternalID += el;
	}
	SetChildModified( el->GetInternalID() );
}

void xoDoc::ChildAddedFromDocumentClone( xoDomEl* el )
{
	xoInternalID elID = el->GetInternalID();
	XOASSERTDEBUG(elID != 0);
	XOASSERTDEBUG(elID < ChildByInternalID.size());		// The clone should have resized ChildByInternalID before copying the DOM elements
	ChildByInternalID[elID] = el;
}

void xoDoc::ChildRemoved( xoDomEl* el )
{
	xoInternalID elID = el->GetInternalID();
	XOASSERT(elID != 0);
	XOASSERT(el->GetDoc() == this);
	IncVersion();
	SetChildModified( elID );
	ChildByInternalID[elID] = NULL;
	el->SetDoc( NULL );
	el->SetInternalID( xoInternalIDNull );
	FreeIDs += elID;
}

void xoDoc::SetChildModified( xoInternalID id )
{
	ChildIsModified.SetAutoGrow( id, true, false );
	IncVersion();
}

void xoDoc::Reset()
{
	/*
	if ( IsReadOnly )
	{
		Root.Discard();
		ClassStyles.Discard();
	}
	*/
	IncVersion();
	Pool.FreeAll();
	Root.SetInternalID( xoInternalIDNull );	// Root will be assigned xoInternalIDRoot when we call ChildAdded() on it.
	ChildIsModified.Clear();
	ResetInternalIDs();
}

void xoDoc::ResetInternalIDs()
{
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += nullptr;	// zero is NULL
	ChildAdded( &Root );
	XOASSERT( Root.GetInternalID() == xoInternalIDRoot );
}

void xoDoc::InitializeDefaultTagStyles()
{
#if XO_PLATFORM_WIN_DESKTOP
	//const char* font = "Trebuchet MS";
	//const char* font = "Microsoft Sans Serif";
	//const char* font = "Consolas";
	//const char* font = "Times New Roman";
	//const char* font = "Verdana";
	//const char* font = "Tahoma";
	const char* font = "Segoe UI";
	//const char* font = "Arial";
#elif XO_PLATFORM_ANDROID
	const char* font = "Droid Sans";
#else
	const char* font = "Helvetica";
#endif
	xoStyleAttrib afont;
	afont.SetFont( xoGlobal()->FontStore->InsertByFacename(font) );

	// Other defaults are set inside xoRenderStack::Initialize()

	TagStyles[xoTagBody].Parse( "background: #fff; width: 100%; height: 100%; box-sizing: margin;", this );
	TagStyles[xoTagBody].Set( afont );
	//TagStyles[xoTagBody].Parse( "background: #000; width: 100%; height: 100%;", this );
	//TagStyles[xoTagDiv].Parse( "display: block;", this );
	// Hack to give text some size
	//TagStyles[xoTagText].Parse( "width: 70px; height: 30px;", this );
	//TagStyles[xoTagLab]...

	static_assert(xoTagCanvas == xoTagEND - 1, "add default style for new tag");
}
