#pragma once
#include "../Style.h"
#include "../Base/MemPoolsAndContainers.h"

namespace xo {

class RenderStack;
class RenderDomEl;
class RenderDomNode;
class RenderDomText;

struct XO_API RenderCharEl {
	int32_t OriginalCharIndex = -1; // Index of first byte of UTF-8 character inside DomEl.Text. Used for selection and UI feedback, such as caret placement inside edit box.
	int32_t Char              = 0;  // Unicode code point
	Pos     X                 = 0;
	Pos     Y                 = 0;
	Pos     Width             = 0;
};

// Element that is ready for rendering
// It's kinda nice that these objects don't have a vtable, because it saves a few bytes on each one,
// but I guess if you need it, then go ahead and add it. But if you can do it statically, then.. do it statically.
class XO_API RenderDomEl {
public:
	RenderDomEl(InternalID id, Tag tag);

	xo::InternalID InternalID; // Reference to our original DomEl. There can be many RenderDomEl per DomEl (text is an example)
	Box            Pos;        // This is the ContentBox, relative to the parent RenderDomEl's context box. See log entry from 2014-08-02
	xo::Tag        Tag;

	bool IsNode() const { return Tag != TagText; }
	bool IsText() const { return Tag == TagText; }
	bool IsCanvas() const { return Tag == TagCanvas; }

	RenderDomNode* ToNode() { return Tag == TagText ? nullptr : reinterpret_cast<RenderDomNode*>(this); }
	RenderDomText* ToText() { return Tag == TagText ? reinterpret_cast<RenderDomText*>(this) : nullptr; }
};

class XO_API RenderDomNode : public RenderDomEl {
public:
	RenderDomNode(xo::InternalID id = InternalIDNull, xo::Tag tag = TagBody, xo::Pool* pool = NULL);

	void    Discard();
	void    SetStyle(RenderStack& stack);
	void    SetPool(Pool* pool);
	float   ContentWidthPx() const { return xo::PosToReal(Pos.Width()); }
	float   ContentHeightPx() const { return xo::PosToReal(Pos.Height()); }
	Box     ContentBox() const { return Pos; }
	Box     BorderBox() const;
	xo::Pos BorderBoxRight() const { return Pos.Right + Style.BorderSize.Right; }
	xo::Pos BorderBoxBottom() const { return Pos.Bottom + Style.BorderSize.Bottom; }

	StyleRender             Style;
	PoolArray<RenderDomEl*> Children;
};

// This is confusing - it should perhaps be named RenderDomWords, because it
// is at a lower level than a DomText object. This represents a bunch of words that
// fit on a single line. A DomText object is actually represented by a RenderDomNode.
// On the other hand, for efficiency sake, one might want to have another level of
// object beneath this, called RenderDomWords, so that we don't redundantly store
// [font, color, fontsize, flags] for every line of text. Probably needless worry though,
// considering that the number of characters in an average line of text will far outweigh
// the storage for the items mentioned above.
class XO_API RenderDomText : public RenderDomEl {
public:
	enum Flag {
		FlagSubPixelGlyphs = 1,
	};
	RenderDomText(xo::InternalID id, Pool* pool);

	bool IsSubPixel() const { return !!(Flags & FlagSubPixelGlyphs); }

	xo::FontID              FontID;
	PoolArray<RenderCharEl> Text;
	xo::Color               Color;
	uint8_t                 FontSizePx;
	uint8_t                 Flags;
};
} // namespace xo
