#pragma once

#include "../xoStyle.h"
#include "../xoMem.h"

class xoRenderStack;

struct XOAPI xoRenderCharEl
{
	int		Char;
	xoPos	X;
	xoPos	Y;
};

// Element that is ready for rendering
class XOAPI xoRenderDomEl
{
public:
	xoRenderDomEl(xoInternalID id, xoTag tag);

	xoInternalID					InternalID;			// Reference to our original xoDomEl. There can be many xoRenderDomEl per xoDomEl (text is an example)
	xoBox							Pos;				// This is the ContentBox, relative to the parent xoRenderDomEl's context box. See log entry from 2014-08-02
	xoTag							Tag;

	bool IsNode() const		{ return Tag != xoTagText; }
	bool IsText() const		{ return Tag == xoTagText; }
	bool IsCanvas() const	{ return Tag == xoTagCanvas; }
};

class XOAPI xoRenderDomNode : public xoRenderDomEl
{
public:
	xoRenderDomNode(xoInternalID id = xoInternalIDNull, xoTag tag = xoTagBody, xoPool* pool = NULL);

	void		Discard();
	void		SetStyle(xoRenderStack& stack);
	void		SetPool(xoPool* pool);
	xoBox		BorderBox() const;
	xoPos		BorderBoxRight() const		{ return Pos.Right + Style.BorderSize.Right; }
	xoPos		BorderBoxBottom() const		{ return Pos.Bottom + Style.BorderSize.Bottom; }

	xoStyleRender					Style;
	xoPoolArray<xoRenderDomEl*>		Children;
};

// This is confusing - it should perhaps be named xoRenderDomWords, because it
// is at a lower level than an xoDomText object. This represents a bunch of words that
// fit on a single line. An xoDomText object is actually represented by an xoRenderDomNode.
// On the other hand, for efficiency sake, one might want to have another level of
// object beneath this, called xoRenderDomWords, so that we don't redundantly store
// [font, color, fontsize, flags] for every line of text. Probably needless worry though,
// considering that the number of characters in an average line of text will far outweigh
// the storage for the items mentioned above.
class XOAPI xoRenderDomText : public xoRenderDomEl
{
public:
	enum Flag
	{
		FlagSubPixelGlyphs = 1,
	};
	xoRenderDomText(xoInternalID id, xoPool* pool);

	void		SetStyle(xoRenderStack& stack);   // get rid of me. Instead just set color manually, the way it's done from xoLayout3

	bool		IsSubPixel() const { return !!(Flags & FlagSubPixelGlyphs); }

	xoFontID						FontID;
	xoPoolArray<xoRenderCharEl>		Text;
	xoColor							Color;
	uint8							FontSizePx;
	uint8							Flags;
};
