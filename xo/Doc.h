#pragma once
#include "Dom/DomNode.h"
#include "Dom/DomText.h"
#include "Base/MemPoolsAndContainers.h"
#include "Containers/StringTable.h"
#include "Containers/StringTableGC.h"
#include "Containers/VariableTable.h"
#include "Image/ImageStore.h"
#include "DocUI.h"

namespace xo {

/* Document.

Version
-------
Every time an event handler runs, we increment the document version number. This is a conservative
way of detecting any change to the document, without requiring that all document updates go via
some kind of accessor function. Our renderer compares its last version to our current version,
and if the two differ, it knows that it needs to update.

InternalID Recycling
--------------------
It's a bit scary how we recycle IDs. For example, I can imagine DocUI.MouseDownID being populated
with an ID that is destroyed and recreated during the time when the mouse is down, which would
produce incorrect behaviour. A safer alternative would be to only recycle IDs once we've wrapped
around 2^32. That would force more complicated code on us, because we could no longer use a
simple lookup table for ID -> Object. I guess time will tell if this is a problem.

*/
class XO_API Doc {
	XO_DISALLOW_COPY_AND_ASSIGN(Doc);

public:
	DomNode     Root;              // Root element of the document tree
	StyleTable  ClassStyles;       // All style classes defined in this document
	Style       TagStyles[TagEND]; // Styles of tags. For example, the style of <p>, or the style of <h1>.
	StringTable Strings;           // Generic string table.
	ImageStore  Images;            // All images. Some day we may want to be able to share these amongst different documents.
	DocUI       UI;                // UI state (which element has the focus, over which elements is the cursor, etc)
	DocGroup*   Group = nullptr;

	Doc(DocGroup* group);
	~Doc();
	void       Reset();
	void       IncVersion();
	uint32_t   GetVersion() { return Version; }                                      // Renderers use purposefully loose thread semantics on this. Valgrind will be unhappy with this.
	void       ResetModifiedBitmap();                                                // Reset the 'is modified' bitmap of all DOM elements and other things, such as the variable table.
	void       MakeFreeIDsUsable();                                                  // All of our dependent renderers have been updated, we can move FreeIDs over to UsableIDs.
	void       CloneSlowInto(Doc& c, uint32_t cloneFlags, RenderStats& stats) const; // Used to make a read-only clone for the renderer. Preserves existing.
	InternalID InternalIDSize() const;                                               // Returns the size of the InternalID table
	//void				CloneFastInto( Doc& c, uint32_t cloneFlags, RenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Starts from scratch.
	DocGroup* GetDocGroup() const { return Group; }

	// Style Classes
	bool        ClassParse(const char* klass, const char* style, size_t styleMaxLen = -1); // Set the class, overwriting any previously set style
	bool        ParseStyleSheet(const char* sheet);                                        // Parse a style sheet
	void        SetStyleVar(const char* var, const char* val);                             // Set a style variable
	const char* StyleVar(const char* var) const;                                           // Get a style variable, or null if undefined

	// Style variables
	int         GetOrCreateStyleVerbatimID(const char* val, size_t len);
	const char* GetStyleVerbatim(int id) const; // Returns null if the ID is invalid

	DomEl* AllocChild(Tag tag, InternalID parentID);
	void   FreeChild(const DomEl* el);

	String Parse(const char* src); // Set the entire document from a single xml-like string. Returns empty string on success, or error message.

	void     NodeGotTimer(InternalID node);
	void     NodeLostTimer(InternalID node);
	uint32_t FastestTimerMS();
	void     ReadyTimers(int64_t nowTicksMS, cheapvec<NodeEventIDPair>& handlers);

	// Register a handler that is called the next time we have finished rendering.
	// These callbacks are called only once.
	void   NodeGotRender(InternalID node);
	void   NodeLostRender(InternalID node);
	void   RenderHandlers(cheapvec<NodeEventIDPair>& handlers);
	size_t AnyRenderHandlers() const { return NodesWithRender.size() != 0; }

	//void				ChildAddedFromDocumentClone( DomEl* el );
	void           ChildAdded(DomEl* el);
	void           ChildRemoved(DomEl* el);
	void           SetChildModified(InternalID id);
	size_t         ChildByInternalIDListSize() const { return ChildByInternalID.size(); }
	const DomEl**  ChildByInternalIDList() const { return (const DomEl**) ChildByInternalID.data; }
	const DomEl*   GetChildByInternalID(InternalID id) const { return ChildByInternalID[id]; } // A NULL result means this child has been deleted
	const DomNode* GetNodeByInternalID(InternalID id) const { return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }
	DomEl*         GetChildByInternalIDMutable(InternalID id) { return ChildByInternalID[id]; }
	DomNode*       GetNodeByInternalIDMutable(InternalID id) { return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }

protected:
	volatile uint32_t      Version;
	Pool                   Pool;       // Used only when making a clone via CloneFast()
	bool                   IsReadOnly; // Read-only clone used for rendering
	cheapvec<DomEl*>       ChildByInternalID;
	cheapvec<bool>         ChildIsModified; // Bit is set if child has been modified since we last synced with the renderer -- TODO - change to proper bitmap
	cheapvec<InternalID>   UsableIDs;       // When we do a render sync, then FreeIDs are moved into UsableIDs
	cheapvec<InternalID>   FreeIDs;
	ohash::set<InternalID> NodesWithTimers; // Set of all nodes that have an OnTimer event handler registered
	ohash::set<InternalID> NodesWithRender; // Set of all nodes that have an OnRender event handler registered
	VariableTable          StyleVariables;
	StringTableGC          StyleVerbatimStrings; // Table of all the verbatim style strings that contain variable references

	void ResetInternalIDs();
	void InitializeDefaultTagStyles();
	void InitializeDefaultControls();
};
}
