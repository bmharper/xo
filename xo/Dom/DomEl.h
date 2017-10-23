#pragma once

#include "../Style.h"
#include "../Base/MemPoolsAndContainers.h"
#include "../Event.h"

namespace xo {

/* DOM node in the tree.
*/
class XO_API DomEl {
public:
	DomEl(Doc* doc, Tag tag, InternalID parentID = InternalIDNull);
	virtual ~DomEl();

	virtual void        SetText(const char* txt)                           = 0; // Ensure first child is TagText, and set it
	virtual const char* GetText() const                                    = 0; // Inverse behaviour of SetText(). Returns empty string if first child is not text.
	virtual void        CloneSlowInto(DomEl& c, uint32_t cloneFlags) const = 0;
	virtual void        ForgetChildren()                                   = 0;

	void           SetText(const std::string& txt) { SetText(txt.c_str()); }
	xo::InternalID GetInternalID() const { return InternalID; }
	xo::Tag        GetTag() const { return Tag; }
	xo::Doc*       GetDoc() const { return Doc; }
	uint32_t       GetVersion() const { return Version; }
	xo::InternalID GetParentID() const { return ParentID; }
	DomNode*       GetParent();
	const DomNode* GetParent() const;

	DomNode*       ToNode();
	DomText*       ToText();
	const DomNode* ToNode() const;
	const DomText* ToText() const;

	//void					CloneFastInto( DomEl& c, Pool* pool, uint32_t cloneFlags ) const;

	void SetInternalID(InternalID id) { InternalID = id; } // Used by Doc during element creation.
	void SetDoc(Doc* doc) { Doc = doc; }                   // Used by Doc during element creation and destruction.
	bool IsNode() const { return Tag != TagText; }
	bool IsText() const { return Tag == TagText; }

protected:
	xo::Doc*       Doc;            // Owning document
	xo::InternalID ParentID;       // Owning node
	xo::InternalID InternalID = 0; // Internal 32-bit ID that is used to keep track of an object (memory address is not sufficient)
	xo::Tag        Tag;            // Tag, such <div>, etc
	uint32_t       Version = 0;    // Monotonic integer used to detect modified nodes

	void IncVersion();
	void CloneSlowIntoBase(DomEl& c, uint32_t cloneFlags) const;
};
} // namespace xo
