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
				xoRenderDomEl( xoInternalID id, xoTag tag );

	xoInternalID					InternalID;			// Reference to our original xoDomEl. There can be many xoRenderDomEl per xoDomEl (text is an example)
	xoBox							Pos;				// For rectangles, this is the ContentBox. See log entry from 2014-08-02
	xoTag							Tag;

	bool IsNode() const		{ return Tag != xoTagText; }
	bool IsText() const		{ return Tag == xoTagText; }
	bool IsCanvas() const	{ return Tag == xoTagCanvas; }
};

class XOAPI xoRenderDomNode : public xoRenderDomEl
{
public:
				xoRenderDomNode( xoInternalID id = xoInternalIDNull, xoTag tag = xoTagBody, xoPool* pool = NULL );

	void		Discard();
	void		SetStyle( xoRenderStack& stack );
	void		SetPool( xoPool* pool );
	xoBox		BorderBox() const;
	xoPos		BorderBoxRight() const		{ return Pos.Right + Style.BorderSize.Right; }
	xoPos		BorderBoxBottom() const		{ return Pos.Bottom + Style.BorderSize.Bottom; }

	xoStyleRender					Style;
	xoPoolArray<xoRenderDomEl*>		Children;
};

class XOAPI xoRenderDomText : public xoRenderDomEl
{
public:
	enum Flag
	{
		FlagSubPixelGlyphs = 1,
	};
				xoRenderDomText( xoInternalID id, xoPool* pool );

	void		SetStyle( xoRenderStack& stack ); // get rid of me. Instead just set color manually, the way it's done from xoLayout3
	
	bool		IsSubPixel() const { return !!(Flags & FlagSubPixelGlyphs); }

	xoFontID						FontID;
	xoPoolArray<xoRenderCharEl>		Text;
	xoColor							Color;
	uint8							FontSizePx;
	uint8							Flags;
};
