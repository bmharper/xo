#include "pch.h"
#include "xoDoc.h"
#include "xoDocGroup.h"
#include "Render/xoRenderer.h"
#include "Text/xoFontStore.h"
#include "xoCloneHelpers.h"
#include "xoStyle.h"
#include "Dom/xoDomCanvas.h"

xoDoc::xoDoc()
	: Root(this, xoTagBody, xoInternalIDNull), UI(this)
{
	IsReadOnly = false;
	Version = 0;
	ClassStyles.AddDummyStyleZero();
	ResetInternalIDs();
	InitializeDefaultTagStyles();
}

xoDoc::~xoDoc()
{
	// TODO: Ensure that all of your events in the process-wide event queue have been dealt with,
	// because the event processor is going to try to access this doc.

	// Wipe all 
	Reset();
}

void xoDoc::IncVersion()
{
	Version++;
}

void xoDoc::ResetModifiedBitmap()
{
	if (ChildIsModified.Size() > 0)
		ChildIsModified.Fill(0, ChildIsModified.Size() - 1, false);
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
void xoDoc::CloneSlowInto(xoDoc& c, uint cloneFlags, xoRenderStats& stats) const
{
	c.IsReadOnly = true;

	// Make sure the destination is large enough to hold all of our children
	while (c.ChildByInternalID.size() < ChildByInternalID.size())
		c.ChildByInternalID += nullptr;

	// Although it would be trivial to parallelize the following two passes, I think it is unlikely to be worth it,
	// since I suspect these passes will be bandwidth limited.

	// Pass 1: Ensure that all objects that are present in the source document have a valid pointer in the target document
	for (int i = 0; i < ChildIsModified.Size(); i++)
	{
		if (ChildIsModified.Get(i))
		{
			stats.Clone_NumEls++;
			const xoDomEl* src = GetChildByInternalID(i);
			xoDomEl* dst = c.GetChildByInternalIDMutable(i);
			if (src && !dst)
			{
				// create in destination
				c.ChildByInternalID[i] = c.AllocChild(src->GetTag(), src->GetParentID());
			}
			else if (!src && dst)
			{
				// destroy destination. Make it forget its children, because this loop takes care of all elements.
				dst->ForgetChildren();
				c.FreeChild(dst);
				c.ChildByInternalID[i] = nullptr;
			}
		}
	}

	// Pass 2: Clone the contents of all our modified objects into our target
	for (int i = 0; i < ChildIsModified.Size(); i++)
	{
		if (ChildIsModified.Get(i))
		{
			const xoDomEl* src = GetChildByInternalID(i);
			xoDomEl* dst = c.GetChildByInternalIDMutable(i);
			if (src)
				src->CloneSlowInto(*dst, cloneFlags);
		}
	}

	ClassStyles.CloneSlowInto(c.ClassStyles);
	xoCloneStaticArrayWithCloneSlowInto(c.TagStyles, TagStyles);

	c.Strings.CloneFrom_Incremental(Strings);

	c.Version = Version;

	UI.CloneSlowInto(c.UI);
}

bool xoDoc::ClassParse(const char* klass, const char* style)
{
	const char* colon = strchr(klass, ':');
	xoString tmpKlass;
	xoString pseudo;
	if (colon != nullptr)
	{
		tmpKlass = klass;
		tmpKlass.Z[colon - klass] = 0;
		pseudo = colon + 1;
		klass = tmpKlass.Z;
	}
	xoStyleClass* s = ClassStyles.GetOrCreate(klass);
	xoStyle* subset = nullptr;
	if (pseudo.Length() == 0)		{ subset = &s->Default; }
	else if (pseudo == "hover")	{ subset = &s->Hover; }
	else if (pseudo == "focus")	{ subset = &s->Focus; }
	else
	{
		return false;
	}
	return subset->Parse(style, this);
}

xoDomEl* xoDoc::AllocChild(xoTag tag, xoInternalID parentID)
{
	XOASSERT(tag != xoTagNULL);

	// we may want to use a more specialized heap for DOM elements in future,
	// so we keep this allocation path strict.
	// In other words, xoDoc is the only thing that may create a new DOM element.
	switch (tag)
	{
	case xoTagText:
		return new xoDomText(this, tag, parentID);
	case xoTagCanvas:
		return new xoDomCanvas(this, parentID);
	default:
		return new xoDomNode(this, tag, parentID);
	}
}

void xoDoc::FreeChild(const xoDomEl* el)
{
	// Same as AllocChild, all DOM elements must be deleted here
	delete el;
}

xoString xoDoc::Parse(const char* src)
{
	return Root.Parse(src);
}

void xoDoc::NodeGotTimer(xoInternalID node)
{
	NodesWithTimers.insert(node);
}

void xoDoc::NodeLostTimer(xoInternalID node)
{
	NodesWithTimers.erase(node);
}

// Return the interval of the fastest timer of any node. Returns zero if no timers are set.
uint xoDoc::FastestTimerMS()
{
	uint fastest = UINT32MAX - 1;
	for (xoInternalID it : NodesWithTimers)
	{
		const xoDomNode* node = GetNodeByInternalID(it);
		uint t = node->FastestTimerMS();
		if (t != 0)
			fastest = xoMin(fastest, t);
	}
	return fastest != UINT32MAX - 1 ? fastest : 0;
}

void xoDoc::ChildAdded(xoDomEl* el)
{
	XOASSERT(el->GetDoc() == this);
	XOASSERT(el->GetInternalID() == 0);
	if (UsableIDs.size() != 0)
	{
		el->SetInternalID(UsableIDs.rpop());
		ChildByInternalID[el->GetInternalID()] = el;
	}
	else
	{
		el->SetInternalID((xoInternalID) ChildByInternalID.size());
		ChildByInternalID += el;
	}
	SetChildModified(el->GetInternalID());
}

/*
void xoDoc::ChildAddedFromDocumentClone( xoDomEl* el )
{
	xoInternalID elID = el->GetInternalID();
	XOASSERTDEBUG(elID != 0);
	XOASSERTDEBUG(elID < ChildByInternalID.size());		// The clone should have resized ChildByInternalID before copying the DOM elements
	ChildByInternalID[elID] = el;
}
*/

void xoDoc::ChildRemoved(xoDomEl* el)
{
	xoInternalID elID = el->GetInternalID();
	XOASSERT(elID != 0);
	XOASSERT(el->GetDoc() == this);
	xoDomNode* node = el->ToNode();
	if (node && node->HandlesEvent(xoEventTimer))
		NodeLostTimer(elID);
	IncVersion();
	SetChildModified(elID);
	ChildByInternalID[elID] = NULL;
	el->SetDoc(NULL);
	el->SetInternalID(xoInternalIDNull);
	FreeIDs += elID;

}

void xoDoc::SetChildModified(xoInternalID id)
{
	ChildIsModified.SetAutoGrow(id, true, false);
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
	Root.SetInternalID(xoInternalIDNull);	// Root will be assigned xoInternalIDRoot when we call ChildAdded() on it.
	ChildIsModified.Clear();
	ResetInternalIDs();
	NodesWithTimers.clear();
}

void xoDoc::ResetInternalIDs()
{
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += nullptr;	// zero is NULL
	ChildAdded(&Root);
	XOASSERT(Root.GetInternalID() == xoInternalIDRoot);
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
	const char* font = "Roboto";
#else
	const char* font = "Droid Sans";
#endif
	xoStyleAttrib afont;
	afont.SetFont(xoGlobal()->FontStore->InsertByFacename(font));

	// Other defaults are set inside xoRenderStack::Initialize()
	// It would be good to establish consistency on where the defaults are set: here or there.
	// The defaults inside xoRenderStack are like a fallback. These are definitely at a "higher" level
	// of abstraction.

	TagStyles[xoTagBody].Parse("background: #fff; width: 100%; height: 100%; box-sizing: margin; cursor: arrow", this);
	TagStyles[xoTagBody].Set(afont);
	TagStyles[xoTagDiv].Parse("baseline:baseline;", this);
	// TagStyles[xoTagText]
	// Setting cursor: text on <lab> is amusing, and it is the default in HTML, but not the right default for general-purpose UI.
	// If this were true here also, then it would imply that all text on a page is selectable.
	TagStyles[xoTagLab].Parse("baseline:baseline", this);
	TagStyles[xoTagSpan].Parse("flow-context: inject; baseline: baseline; bump: horizontal", this);
	//TagStyles[xoTagCanvas]

	static_assert(xoTagCanvas == xoTagEND - 1, "add default style for new tag");
}
