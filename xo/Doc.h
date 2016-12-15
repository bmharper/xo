#pragma once
#include "Dom/DomNode.h"
#include "Dom/DomText.h"
#include "Mem.h"
#include "StringTable.h"
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

*/
class XO_API Doc {
	DISALLOW_COPY_AND_ASSIGN(Doc);

public:
	DomNode     Root;              // Root element of the document tree
	StyleTable  ClassStyles;       // All style classes defined in this document
	Style       TagStyles[TagEND]; // Styles of tags. For example, the style of <p>, or the style of <h1>.
	StringTable Strings;           // Generic string table.
	ImageStore  Images;            // All images. Some day we may want to be able to share these amongst different documents.
	DocUI       UI;                // UI state (which element has the focus, over which elements is the cursor, etc)

	Doc();
	~Doc();
	void   Reset();
	void   IncVersion();
	uint32_t GetVersion() { return Version; }                                  // Renderers use purposefully loose thread semantics on this.
	void   ResetModifiedBitmap();                                            // Reset the 'ismodified' bitmap of all DOM elements.
	void   MakeFreeIDsUsable();                                              // All of our dependent renderers have been updated, we can move FreeIDs over to UsableIDs.
	void   CloneSlowInto(Doc& c, uint32_t cloneFlags, RenderStats& stats) const; // Used to make a read-only clone for the renderer. Preserves existing.
	//void				CloneFastInto( Doc& c, uint32_t cloneFlags, RenderStats& stats ) const;	// Used to make a read-only clone for the renderer. Starts from scratch.

	// Style Classes
	bool ClassParse(const char* klass, const char* style); // Set the class, overwriting any previously set style

	DomEl* AllocChild(Tag tag, InternalID parentID);
	void   FreeChild(const DomEl* el);

	String Parse(const char* src); // Set the entire document from a single xml-like string. Returns empty string on success, or error message.

	void NodeGotTimer(InternalID node);
	void NodeLostTimer(InternalID node);
	uint32_t FastestTimerMS();

	void ChildAdded(DomEl* el);
	//void				ChildAddedFromDocumentClone( DomEl* el );
	void           ChildRemoved(DomEl* el);
	void           SetChildModified(InternalID id);
	size_t         ChildByInternalIDListSize() const { return ChildByInternalID.size(); }
	const DomEl**  ChildByInternalIDList() const { return (const DomEl**) ChildByInternalID.data; }
	const DomEl*   GetChildByInternalID(InternalID id) const { return ChildByInternalID[id]; } // A NULL result means this child has been deleted
	const DomNode* GetNodeByInternalID(InternalID id) const { return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }
	DomEl*         GetChildByInternalIDMutable(InternalID id) { return ChildByInternalID[id]; }
	DomNode*       GetNodeByInternalIDMutable(InternalID id) { return ChildByInternalID[id] ? ChildByInternalID[id]->ToNode() : nullptr; }

protected:
	volatile uint32_t        Version;
	Pool                   Pool;       // Used only when making a clone via CloneFast()
	bool                   IsReadOnly; // Read-only clone used for rendering
	cheapvec<DomEl*>          ChildByInternalID;
	BitMap                 ChildIsModified; // Bit is set if child has been modified since we last synced with the renderer
	cheapvec<InternalID>     UsableIDs;       // When we do a render sync, then FreeIDs are moved into UsableIDs
	cheapvec<InternalID>     FreeIDs;
	ohash::set<InternalID> NodesWithTimers; // Set of all nodes that have an OnTimer event handler registered

	void ResetInternalIDs();
	void InitializeDefaultTagStyles();
};
}
