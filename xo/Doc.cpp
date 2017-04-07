#include "pch.h"
#include "Doc.h"
#include "DocGroup.h"
#include "Render/Renderer.h"
#include "Text/FontStore.h"
#include "CloneHelpers.h"
#include "Style.h"
#include "Dom/DomCanvas.h"
#include "Controls/EditBox.h"
#include "Controls/Button.h"
#include "Controls/MsgBox.h"

namespace xo {

Doc::Doc(DocGroup* group)
    : Root(this, TagBody, InternalIDNull), UI(this), Group(group), StyleVariables(this), VectorIcons(this) {
	IsReadOnly = false;
	Version    = 0;
	ClassStyles.AddDummyStyleZero();
	ResetInternalIDs();
	InitializeDefaultTagStyles();
	InitializeDefaultControls();
}

Doc::~Doc() {
	// TODO: Ensure that all of your events in the process-wide event queue have been dealt with,
	// because the event processor is going to try to access this doc.

	// Wipe all
	Reset();
}

void Doc::IncVersion() {
	Version++;
}

void Doc::ResetModifiedBitmap() {
	if (ChildIsModified.size() != 0)
		ChildIsModified.fill(false);
	StyleVariables.ResetModified();
	StyleVerbatimStrings.ResetModified();
}

void Doc::MakeFreeIDsUsable() {
	UsableIDs += FreeIDs;
	FreeIDs.clear();
}

// This clones only the objects that are marked as modified.
void Doc::CloneSlowInto(Doc& c, uint32_t cloneFlags, RenderStats& stats) const {
	c.IsReadOnly = true;

	// Make sure the destination is large enough to hold all of our children
	while (c.ChildByInternalID.size() < ChildByInternalID.size())
		c.ChildByInternalID += nullptr;

	// Although it would be trivial to parallelize the following two passes, I think it is unlikely to be worth it,
	// since I suspect these passes will be bandwidth limited.

	// Pass 1: Ensure that all objects that are present in the source document have a valid pointer in the target document
	for (size_t i = 0; i < ChildIsModified.size(); i++) {
		if (ChildIsModified[i]) {
			stats.Clone_NumEls++;
			const DomEl* src = GetChildByInternalID((xo::InternalID) i);
			DomEl*       dst = c.GetChildByInternalIDMutable((xo::InternalID) i);
			if (src && !dst) {
				// create in destination
				c.ChildByInternalID[i] = c.AllocChild(src->GetTag(), src->GetParentID());
			} else if (!src && dst) {
				// destroy destination. Make it forget its children, because this loop takes care of all elements.
				dst->ForgetChildren();
				c.FreeChild(dst);
				c.ChildByInternalID[i] = nullptr;
			}
		}
	}

	// Pass 2: Clone the contents of all our modified objects into our target
	for (size_t i = 0; i < ChildIsModified.size(); i++) {
		if (ChildIsModified[i]) {
			const DomEl* src = GetChildByInternalID((xo::InternalID) i);
			DomEl*       dst = c.GetChildByInternalIDMutable((xo::InternalID) i);
			if (src)
				src->CloneSlowInto(*dst, cloneFlags);
		}
	}

	ClassStyles.CloneSlowInto(c.ClassStyles);
	CloneStaticArrayWithCloneSlowInto(c.TagStyles, TagStyles);

	c.StyleVariables.CloneFrom_Incremental(StyleVariables);
	c.Strings.CloneFrom_Incremental(Strings);
	c.StyleVerbatimStrings.CloneFrom_Incremental(StyleVerbatimStrings);
	
	c.VectorIcons.CloneFrom_Incremental(VectorIcons);

	c.Version = Version;

	UI.CloneSlowInto(c.UI);
}

InternalID Doc::InternalIDSize() const {
	return (InternalID) ChildByInternalID.size();
}

bool Doc::ClassParse(const char* klass, const char* style, size_t styleMaxLen) {
	const char* colon = strchr(klass, ':');
	String      tmpKlass;
	String      pseudo;
	if (colon != nullptr) {
		tmpKlass                  = klass;
		tmpKlass.Z[colon - klass] = 0;
		pseudo                    = colon + 1;
		klass                     = tmpKlass.CStr();
	}
	StyleClass* s      = ClassStyles.GetOrCreate(klass);
	Style*      subset = nullptr;
	if (pseudo.Length() == 0) {
		subset = &s->Default;
	} else if (pseudo == "hover") {
		subset = &s->Hover;
	} else if (pseudo == "focus") {
		subset = &s->Focus;
	} else if (pseudo == "capture") {
		subset = &s->Capture;
	} else {
		return false;
	}
	return subset->Parse(style, styleMaxLen, this);
}

bool Doc::ParseStyleSheet(const char* sheet) {
	return Style::ParseSheet(sheet, this);
}

void Doc::SetStyleVar(const char* var, const char* val) {
	XO_ASSERT(strlen(var) <= MaxStyleVarNameLen);
	StyleVariables.Set(var, val);
}

const char* Doc::StyleVar(const char* var) const {
	return StyleVariables.GetByName(var);
}

int Doc::GetOrCreateStyleVerbatimID(const char* val, size_t len) {
	return StyleVerbatimStrings.GetOrCreateID(val, len);
}

const char* Doc::GetStyleVerbatim(int id) const {
	return StyleVerbatimStrings.GetStr(id);
}

int Doc::SetSvg(const char* name, const char* val) {
	return VectorIcons.Set(name, val);
}

int Doc::GetSvgID(const char* name) const {
	return VectorIcons.GetID(name);
}

const char* Doc::GetSvg(int id) const {
	return VectorIcons.GetByID(id);
}

DomEl* Doc::AllocChild(Tag tag, InternalID parentID) {
	XO_ASSERT(tag != TagNULL);

	// we may want to use a more specialized heap for DOM elements in future,
	// so we keep this allocation path strict.
	// In other words, Doc is the only thing that may create a new DOM element.
	switch (tag) {
	case TagText:
		return new DomText(this, tag, parentID);
	case TagCanvas:
		return new DomCanvas(this, parentID);
	default:
		return new DomNode(this, tag, parentID);
	}
}

void Doc::FreeChild(const DomEl* el) {
	// As the inverse of AllocChild, all DOM elements must be deleted by this function
	delete el;
}

String Doc::Parse(const char* src) {
	return Root.Parse(src);
}

void Doc::NodeGotTimer(InternalID node) {
	NodesWithTimers.insert(node);
}

void Doc::NodeLostTimer(InternalID node) {
	NodesWithTimers.erase(node);
}

// Return the interval of the fastest timer of any node. Returns zero if no timers are set.
uint32_t Doc::FastestTimerMS() {
	uint32_t fastest = UINT32_MAX - 1;
	for (InternalID it : NodesWithTimers) {
		const DomNode* node = GetNodeByInternalID(it);
		uint32_t       t    = node->FastestTimerMS();
		if (t != 0)
			fastest = Min(fastest, t);
	}
	return fastest != UINT32_MAX - 1 ? fastest : 0;
}

// Returns all nodes that have timers which have elapsed
void Doc::ReadyTimers(int64_t nowTicksMS, cheapvec<NodeEventIDPair>& handlers) {
	for (InternalID id : NodesWithTimers) {
		DomNode* node = GetNodeByInternalIDMutable(id);
		node->ReadyTimers(nowTicksMS, handlers);
	}
}

void Doc::NodeGotRender(InternalID node) {
	NodesWithRender.insert(node);
}

void Doc::NodeLostRender(InternalID node) {
	NodesWithRender.erase(node);
}

void Doc::RenderHandlers(cheapvec<NodeEventIDPair>& handlers) {
	for (InternalID id : NodesWithRender) {
		DomNode* node = GetNodeByInternalIDMutable(id);
		node->RenderHandlers(handlers);
	}
}

void Doc::ChildAdded(DomEl* el) {
	XO_ASSERT(el->GetDoc() == this);
	XO_ASSERT(el->GetInternalID() == 0);
	if (UsableIDs.size() != 0) {
		el->SetInternalID(UsableIDs.rpop());
		ChildByInternalID[el->GetInternalID()] = el;
	} else {
		el->SetInternalID((InternalID) ChildByInternalID.size());
		ChildByInternalID += el;
	}
	SetChildModified(el->GetInternalID());
}

/*
void Doc::ChildAddedFromDocumentClone( DomEl* el )
{
	InternalID elID = el->GetInternalID();
	XO_DEBUG_ASSERT(elID != 0);
	XO_DEBUG_ASSERT(elID < ChildByInternalID.size());		// The clone should have resized ChildByInternalID before copying the DOM elements
	ChildByInternalID[elID] = el;
}
*/

void Doc::ChildRemoved(DomEl* el) {
	InternalID elID = el->GetInternalID();
	XO_ASSERT(elID != 0);
	XO_ASSERT(el->GetDoc() == this);
	DomNode* node = el->ToNode();
	if (node) {
		if (node->HandlesEvent(EventTimer))
			NodeLostTimer(elID);
		if (node->HandlesEvent(EventRender))
			NodeLostRender(elID);
	}
	IncVersion();
	SetChildModified(elID);
	ChildByInternalID[elID] = NULL;
	// The following pair is actually a bad idea, because some objects (Canvas) need to know their Doc to clean up after themselves.
	//el->SetDoc(NULL);
	//el->SetInternalID(InternalIDNull);
	FreeIDs += elID;
}

void Doc::SetChildModified(InternalID id) {
	size_t index = id;
	while (ChildIsModified.size() <= index)
		ChildIsModified.push_back(false);
	ChildIsModified[index] = true;
	IncVersion();
}

void Doc::Reset() {
	/*
	if ( IsReadOnly )
	{
		Root.Discard();
		ClassStyles.Discard();
	}
	*/
	IncVersion();
	Pool.FreeAll();
	Root.SetInternalID(InternalIDNull); // Root will be assigned InternalIDRoot when we call ChildAdded() on it.
	ChildIsModified.clear();
	ResetInternalIDs();
	NodesWithTimers.clear();
}

void Doc::ResetInternalIDs() {
	FreeIDs.clear();
	UsableIDs.clear();
	ChildByInternalID.clear();
	ChildByInternalID += nullptr; // zero is NULL
	ChildAdded(&Root);
	XO_ASSERT(Root.GetInternalID() == InternalIDRoot);
}

void Doc::InitializeDefaultTagStyles() {
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
	StyleAttrib afont;
	afont.SetFont(Global()->FontStore->InsertByFacename(font));

	// Other defaults are set inside RenderStack::Initialize()
	// It would be good to establish consistency on where the defaults are set: here or there.
	// The defaults inside RenderStack are like a fallback. These are definitely at a "higher" level
	// of abstraction.

	TagStyles[TagBody].Parse("background: #fff; box-sizing: margin; cursor: arrow", this);
	TagStyles[TagBody].Parse("width: 100%; height: 100%", this);
	TagStyles[TagBody].Set(afont);
	// Setting cursor: text on <lab> is amusing, and it is the default in HTML, but not the right default for general-purpose UI.
	// If this were true here also, then it would imply that all text on a page is selectable.
	TagStyles[TagLab].Parse("baseline:baseline", this);
	TagStyles[TagSpan].Parse("flow-context: inject; baseline: baseline; bump: horizontal", this);
	TagStyles[TagCanvas].Parse("background: #fff", this);

	static_assert(TagImg == TagEND - 1, "add default style for new tag");
}

void Doc::InitializeDefaultControls() {
	controls::EditBox::InitializeStyles(this);
	controls::Button::InitializeStyles(this);
	controls::MsgBox::InitializeStyles(this);
}
}
