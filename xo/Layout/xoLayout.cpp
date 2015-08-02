#include "pch.h"
#include "xoLayout.h"
#include "xoDoc.h"
#include "Dom/xoDomNode.h"
#include "Dom/xoDomText.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoStyleResolve.h"
#include "Text/xoFontStore.h"
#include "Text/xoGlyphCache.h"

/* This is called serially.

Why do we perform layout in multiple passes, loading all missing glyphs at the end of each pass?
The reason is because we eventually want to be able to parallelize layout.

Missing glyphs are a once-off cost (ie once per application instance),
so it's not worth trying to use a mutable glyph cache.

*/
void xoLayout::Layout(const xoDoc& doc, u32 docWidth, u32 docHeight, xoRenderDomNode& root, xoPool* pool)
{
	Doc = &doc;
	DocWidth = docWidth;
	DocHeight = docHeight;
	Pool = pool;
	Stack.Initialize(Doc, Pool);
	Fonts = xoGlobal()->FontStore->GetImmutableTable();

	while (true)
	{
		LayoutInternal(root);

		if (GlyphsNeeded.size() == 0)
		{
			XOTRACE_LAYOUT_VERBOSE("Layout done\n");
			break;
		}
		else
		{
			XOTRACE_LAYOUT_VERBOSE("Layout done (but need another pass for missing glyphs)\n");
			RenderGlyphsNeeded();
		}
	}
}

void xoLayout::LayoutInternal(xoRenderDomNode& root)
{
	PtToPixel = 1.0;	// TODO
	EpToPixel = xoGlobal()->EpToPixel;

	XOTRACE_LAYOUT_VERBOSE("Layout 1\n");

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	XOTRACE_LAYOUT_VERBOSE("Layout 2\n");

	NodeState s;
	memset(&s, 0, sizeof(s));
	s.ParentContentBox.SetInt(0, 0, DocWidth, DocHeight);
	//s.PositionedAncestor = s.ParentContentBox;
	s.ParentContentBoxHasWidth = true;
	s.ParentContentBoxHasHeight = true;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;
	s.PosMaxX = s.PosX;
	s.PosMaxY = s.PosY;
	s.PosBaselineY = xoPosNULL;

	XOTRACE_LAYOUT_VERBOSE("Layout 3 DocBox = %d,%d,%d,%d\n", s.ParentContentBox.Left, s.ParentContentBox.Top, s.ParentContentBox.Right, s.ParentContentBox.Bottom);

	RunNode(s, Doc->Root, &root);
}

void xoLayout::RunNode(NodeState& s, const xoDomNode& node, xoRenderDomNode* rnode)
{
	XOTRACE_LAYOUT_VERBOSE("Layout (%d) Run 1\n", node.GetInternalID());
	xoStyleResolver::ResolveAndPush(Stack, &node);
	rnode->SetStyle(Stack);

	XOTRACE_LAYOUT_VERBOSE("Layout (%d) Run 2\n", node.GetInternalID());
	rnode->InternalID = node.GetInternalID();
	if (rnode->InternalID == 50)
		int abc = 123;

	//auto display = Stack.Get( xoCatDisplay ).GetDisplayType();
	auto position = Stack.Get(xoCatPosition).GetPositionType();
	auto boxSizing = Stack.Get(xoCatBoxSizing).GetBoxSizing();
	xoPos contentWidth = ComputeDimension(s.ParentContentBox.Width(), s.ParentContentBoxHasWidth, xoCatWidth);
	xoPos contentHeight = ComputeDimension(s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, xoCatHeight);
	xoPos borderRadius = ComputeDimension(0, false, xoCatBorderRadius);
	xoBox margin = ComputeBox(s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, xoCatMargin_Left);
	xoBox padding = ComputeBox(s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, xoCatPadding_Left);
	xoBox border = ComputeBox(s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, xoCatBorder_Left);

	// It might make for less arithmetic if we work with marginBoxWidth and marginBoxHeight instead of contentBoxWidth and contentBoxHeight. We'll see.
	bool haveWidth = contentWidth != xoPosNULL;
	bool haveHeight = contentHeight != xoPosNULL;
	rnode->Style.BorderRadius = xoPosToReal(borderRadius);

	if (boxSizing == xoBoxSizeContent) {}
	else if (boxSizing == xoBoxSizeBorder)
	{
		if (haveWidth)	contentWidth -= border.Left + border.Right + padding.Left + padding.Right;
		if (haveHeight)	contentHeight -= border.Top + border.Bottom + padding.Top + padding.Bottom;
	}
	else if (boxSizing == xoBoxSizeMargin)
	{
		if (haveWidth)	contentWidth -= margin.Left + margin.Right + border.Left + border.Right + padding.Left + padding.Right;
		if (haveHeight)	contentHeight -= margin.Top + margin.Bottom + border.Top + border.Bottom + padding.Top + padding.Bottom;
	}

	xoBox marginBox;
	if (position == xoPositionAbsolute)
	{
		marginBox = ComputeSpecifiedPosition(s);
	}
	else
	{
		marginBox.Left = marginBox.Right = s.PosX;
		marginBox.Top = marginBox.Bottom = s.PosY;
		if (haveWidth) marginBox.Right += margin.Left + border.Left + padding.Left + contentWidth + padding.Right + border.Right + margin.Right;
		if (haveHeight) marginBox.Bottom += margin.Top + border.Top + padding.Top + contentHeight + padding.Bottom + border.Bottom + margin.Bottom;
	}

	// Check if this block overflows. If we don't have width yet, then we'll check overflow after laying out our children
	if (position == xoPositionStatic && s.ParentContentBoxHasWidth && haveWidth)
		PositionBlock(s, marginBox);

	bool enableRecursiveLayout = false;

	// cs: child state
	NodeState cs;
	for (int pass = 0; true; pass++)
	{
		XOASSERTDEBUG(pass < 2);

		xoBox contentBox = marginBox;
		contentBox.Left += border.Left + padding.Left + margin.Left;
		contentBox.Top += border.Top + padding.Top + margin.Top;
		if (haveWidth)	contentBox.Right -= border.Right + padding.Right + margin.Right;
		else				contentBox.Right = contentBox.Left;
		if (haveHeight)	contentBox.Bottom -= border.Bottom + padding.Bottom + margin.Bottom;
		else				contentBox.Bottom = contentBox.Top;

		cs = s;
		cs.ParentContentBox = contentBox;
		cs.ParentContentBoxHasWidth = haveWidth;
		cs.ParentContentBoxHasHeight = haveHeight;
		cs.PosX = cs.PosMaxX = contentBox.Left;
		cs.PosY = cs.PosMaxY = contentBox.Top;
		cs.PosBaselineY = s.PosBaselineY;

		XOTRACE_LAYOUT_VERBOSE("Layout (%d) Run 3 (position = %d) (%d %d)\n", node.GetInternalID(), (int) position, s.ParentContentBoxHasWidth ? 1 : 0, haveWidth ? 1 : 0);

		const pvect<xoDomEl*>& nodeChildren = node.GetChildren();
		for (int i = 0; i < nodeChildren.size(); i++)
		{
			const xoDomEl* child = nodeChildren[i];
			if (child->GetTag() == xoTagText)
			{
				xoRenderDomText* rchild = new(Pool->AllocT<xoRenderDomText>(false)) xoRenderDomText(child->GetInternalID(), Pool);
				rnode->Children += rchild;
				RunText(cs, *static_cast<const xoDomText*>(child), rchild);
			}
			else
			{
				xoRenderDomNode* rchild = new(Pool->AllocT<xoRenderDomNode>(false)) xoRenderDomNode(child->GetInternalID(), child->GetTag(), Pool);
				rnode->Children += rchild;
				RunNode(cs, *static_cast<const xoDomNode*>(child), rchild);
			}
		}

		if (!haveWidth)	marginBox.Right = cs.PosMaxX + padding.Right + border.Right + margin.Right;
		if (!haveHeight)	marginBox.Bottom = cs.PosMaxY + padding.Bottom + border.Bottom + margin.Bottom;

		// Since our width was undefined, we couldn't check for overflow until we'd layed out our children.
		// If we do overflow now, then we need to retrofit all of our child boxes with an offset.
		if (enableRecursiveLayout && position == xoPositionStatic && s.ParentContentBoxHasWidth && !haveWidth)
		{
			xoPoint offset = PositionBlock(s, marginBox);
			if (offset != xoPoint(0,0))
			{
				haveWidth = true;
				//haveHeight = true;		// We do in fact NOT know height, because it can be altered by the resetting of the baseline on the new line
				rnode->Children.clear();
				continue;
			}
		}
		else
		{
			s.PosX = marginBox.Right;
		}

		rnode->Pos = marginBox.ShrunkBy(margin);
		break;
	}

	XOTRACE_LAYOUT_VERBOSE("Layout (%d) marginBox: %d,%d,%d,%d\n", node.GetInternalID(), marginBox.Left, marginBox.Top, marginBox.Right, marginBox.Bottom);

	s.PosMaxY = xoMax(s.PosMaxY, marginBox.Bottom);
	if (s.PosBaselineY == xoPosNULL)
		s.PosBaselineY = cs.PosBaselineY;

	Stack.StackPop();
}

void xoLayout::RunText(NodeState& s, const xoDomText& node, xoRenderDomText* rnode)
{
	XOTRACE_LAYOUT_VERBOSE("Layout text (%d) Run 1\n", node.GetInternalID());
	rnode->InternalID = node.GetInternalID();
	rnode->SetStyle(Stack);

	XOTRACE_LAYOUT_VERBOSE("Layout text (%d) Run 2\n", node.GetInternalID());

	rnode->FontID = Stack.Get(xoCatFontFamily).GetFont();

	xoStyleAttrib fontSizeAttrib = Stack.Get(xoCatFontSize);
	xoPos fontHeight = ComputeDimension(s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, fontSizeAttrib.GetSize());

	float fontSizePxUnrounded = xoPosToReal(fontHeight);

	// round font size to integer units
	rnode->FontSizePx = (uint8) xoRound(fontSizePxUnrounded);

	// Nothing prevents somebody from setting a font size to zero
	if (rnode->FontSizePx < 1)
		return;

	bool subPixel = xoGlobal()->EnableSubpixelText && rnode->FontSizePx <= xoGlobal()->MaxSubpixelGlyphSize;
	if (subPixel)
		rnode->Flags |= xoRenderDomText::FlagSubPixelGlyphs;

	TempText.Node = &node;
	TempText.RNode = rnode;
	TempText.Words.clear_noalloc();
	TempText.GlyphCount = 0;
	GenerateTextWords(s, TempText);
	if (!TempText.GlyphsNeeded)
		GenerateTextOutput(s, TempText);
}

void xoLayout::GenerateTextWords(NodeState& s, TextRunState& ts)
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts.RNode);

	ts.GlyphsNeeded = false;
	bool onSpace = false;
	Word word;
	word.Start = 0;
	word.Width = 0;
	for (intp i = 0; true; i++)
	{
		bool isSpace = IsSpace(txt[i]) || IsLinebreak(txt[i]);
		if (isSpace || onSpace || txt[i] == 0)
		{
			word.End = (int32) i;
			if (word.End != word.Start)
				ts.Words += word;
			word.Start = (int32) i;
			word.Width = 0;
			onSpace = isSpace;
		}
		if (txt[i] == 0)
			break;
		key.Char = txt[i];
		const xoGlyph* glyph = glyphCache->GetGlyph(key);
		if (!glyph)
		{
			ts.GlyphsNeeded = true;
			GlyphsNeeded.insert(key);
			continue;
		}
		if (glyph->IsNull())
		{
			// TODO: Handle missing glyph by drawing a rectangle or something
			continue;
		}
		ts.GlyphCount++;
		word.Width += xoRealToPos(glyph->MetricLinearHoriAdvance);
	}
}

/*

This diagram was created using asciiflow (http://asciiflow.com/)

               +–––––––––––––––––––––––––––––––––––+  XXXXXXXXXXXX
            XX |      X       X                    |             X
            X  |      X       X                    |             X
            X  |      X       X                    |             X
            X  |      X       X                    |             X
 ascender   X  |      XXXXXXXXX     XXXXXXXX       |             X
            X  |      X       X     X      X       |             X
            X  |      X       X     X      X       |             X lineheight
            XX |      X       X     XXXXXXXX       |             X
               +–––––––––––––––––––+X+––––––––––––––+ baseline   X
            XX |                    X              |             X
descender   X  |                    X              |             X
            X  |                    X              |             X
            XX |                    X              |             X
               |                                   |             X
               +–––––––––––––––––––––––––––––––––––+  XXXXXXXXXXXX


*/
void xoLayout::GenerateTextOutput(NodeState& s, TextRunState& ts)
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts.RNode);
	const xoFont* font = Fonts.GetByFontID(ts.RNode->FontID);

	xoPos fontHeightRounded = xoRealToPos(ts.RNode->FontSizePx);
	xoPos fontAscender = xoRealx256ToPos(font->Ascender_x256 * ts.RNode->FontSizePx);
	xoTextAlignVertical valign = Stack.Get(xoCatText_Align_Vertical).GetTextAlignVertical();

	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos(ts.RNode->FontSizePx * font->LineHeight_x256);
	if (xoGlobal()->RoundLineHeights)
		lineHeight = xoPosRoundUp(lineHeight);

	int fontHeightPx = ts.RNode->FontSizePx;
	ts.RNode->Text.reserve(ts.GlyphCount);
	bool parentHasWidth = s.ParentContentBoxHasWidth;
	bool enableKerning = xoGlobal()->EnableKerning;

	xoPos baseline = 0;
	if (valign == xoTextAlignVerticalTop || s.PosBaselineY == xoPosNULL)		baseline = s.PosY + fontAscender;
	else if (valign == xoTextAlignVerticalBaseline)							baseline = s.PosBaselineY;
	else																		XOTODO;

	// First text in the line defines the baseline
	if (s.PosBaselineY == xoPosNULL)
		s.PosBaselineY = baseline;

	for (intp iword = 0; iword < ts.Words.size(); iword++)
	{
		const Word& word = ts.Words[iword];
		bool isSpace = word.Length() == 1 && txt[word.Start] == 32;
		bool isNewline = word.Length() == 1 && txt[word.Start] == '\n';
		bool over = parentHasWidth ? s.PosX + word.Width > s.ParentContentBox.Right : false;
		if (over)
		{
			bool futile = s.PosX == s.ParentContentBox.Left && word.Width > s.ParentContentBox.Width();
			if (!futile)
			{
				NextLine(s);
				baseline = s.PosY + fontAscender;
				// If the line break was performed for a space, then treat that space as "done"
				if (isSpace)
					continue;
			}
		}

		if (isSpace)
		{
			s.PosX += xoRealx256ToPos(font->LinearHoriAdvance_Space_x256) * fontHeightPx;
		}
		else if (isNewline)
		{
			NextLine(s);
			baseline = s.PosY + fontAscender;
		}
		else
		{
			const xoGlyph* prevGlyph = nullptr;
			for (intp i = word.Start; i < word.End; i++)
			{
				key.Char = txt[i];
				const xoGlyph* glyph = glyphCache->GetGlyph(key);
				__analysis_assume(glyph != nullptr);
				if (glyph->IsNull())
					continue;
				if (enableKerning && prevGlyph)
				{
					// Multithreading hazard here. I'm not sure whether FT_Get_Kerning is thread safe.
					// Also, I have stepped inside there and I see it does a binary search. We would probably
					// be better off caching the kerning for frequent pairs of glyphs in a hash table.
					FT_Vector kern;
					FT_Get_Kerning(font->FTFace, prevGlyph->FTGlyphIndex, glyph->FTGlyphIndex, FT_KERNING_UNSCALED, &kern);
					xoPos kerning = ((kern.x * fontHeightPx) << xoPosShift) / font->FTFace->units_per_EM;
					s.PosX += kerning;
				}
				ts.RNode->Text.Count++;
				xoRenderCharEl& rtxt = ts.RNode->Text.back();
				rtxt.Char = key.Char;
				rtxt.X = s.PosX + xoRealx256ToPos(glyph->MetricLeftx256);
				rtxt.Y = baseline - xoRealToPos(glyph->MetricTop);			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
				s.PosX += xoRealToPos(glyph->MetricLinearHoriAdvance);
				s.PosMaxX = xoMax(s.PosMaxX, s.PosX);
				prevGlyph = glyph;
			}
		}
		s.PosMaxY = xoMax(s.PosMaxY, baseline - fontAscender + lineHeight);
	}
}

void xoLayout::NextLine(NodeState& s)
{
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.PosMaxY;
	s.PosBaselineY = xoPosNULL;
}

xoPoint xoLayout::PositionBlock(NodeState& s, xoBox& marginBox)
{
	XOASSERTDEBUG(s.ParentContentBoxHasWidth);

	xoPoint offset(0,0);

	// Going to next line is futile if this block is as far to the left as possible
	const bool futile = marginBox.Left == s.ParentContentBox.Left;
	if (marginBox.Right > s.ParentContentBox.Right && !futile)
	{
		// Block does not fit on this line, it must move onto the next line.
		XOTRACE_LAYOUT_VERBOSE("Layout block does not fit %d,%d\n", s.PosX, s.PosY);
		NextLine(s);
		offset = xoPoint(s.PosX - marginBox.Left, s.PosY - marginBox.Top);
		marginBox.Offset(offset.X, offset.Y);
	}
	else
	{
		XOTRACE_LAYOUT_VERBOSE("Layout block fits %d,%d\n", s.PosX, s.PosY);
	}
	s.PosX = marginBox.Right;
	return offset;
}

void xoLayout::OffsetRecursive(xoRenderDomNode* rnode, xoPoint offset)
{
	rnode->Pos.Offset(offset);
	for (intp i = 0; i < rnode->Children.size(); i++)
	{
		if (rnode->Children[i]->Tag == xoTagText)
		{
			xoRenderDomText* txt = static_cast<xoRenderDomText*>(rnode->Children[i]);
			for (intp j = 0; j < txt->Text.size(); j++)
			{
				txt->Text[j].X += offset.X;
				txt->Text[j].Y += offset.Y;
			}
		}
		else
		{
			xoRenderDomNode* node = static_cast<xoRenderDomNode*>(rnode->Children[i]);
			OffsetRecursive(node, offset);
		}
	}
}

bool xoLayout::IsSpace(int ch)
{
	return ch == 32;
}

bool xoLayout::IsLinebreak(int ch)
{
	return ch == '\r' || ch == '\n';
}

xoGlyphCacheKey	xoLayout::MakeGlyphCacheKey(xoRenderDomText* rnode)
{
	uint8 glyphFlags = rnode->IsSubPixel() ? xoGlyphFlag_SubPixel_RGB : 0;
	return xoGlyphCacheKey(rnode->FontID, 0, rnode->FontSizePx, glyphFlags);
}

void xoLayout::RenderGlyphsNeeded()
{
	for (auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++)
		xoGlobal()->GlyphCache->RenderGlyph(*it);
	GlyphsNeeded.clear();
}

xoPos xoLayout::ComputeDimension(xoPos container, bool isContainerDefined, xoStyleCategories cat)
{
	return ComputeDimension(container, isContainerDefined, Stack.Get(cat).GetSize());
}

xoPos xoLayout::ComputeDimension(xoPos container, bool isContainerDefined, xoSize size)
{
	switch (size.Type)
	{
	case xoSize::NONE: return xoPosNULL;
	case xoSize::PX: return xoRealToPos(size.Val);
	case xoSize::PT: return xoRealToPos(size.Val * PtToPixel);
	case xoSize::EP: return xoRealToPos(size.Val * EpToPixel);
	case xoSize::PERCENT:
		if (container == xoPosNULL || !isContainerDefined)
			return xoPosNULL;
		else
			return xoPos((float) container * (size.Val * 0.01f));
	default: XOPANIC("Unrecognized size type"); return 0;
	}
}

xoBox xoLayout::ComputeSpecifiedPosition(const NodeState& s)
{
	//xoBox reference = s.PositionedAncestor;
	xoBox reference = s.ParentContentBox;
	xoBox box = reference;
	bool refWidthDefined = s.ParentContentBoxHasWidth;
	bool refHeightDefined = s.ParentContentBoxHasHeight;
	auto left = Stack.Get(xoCatLeft);
	auto right = Stack.Get(xoCatRight);
	auto top = Stack.Get(xoCatTop);
	auto bottom = Stack.Get(xoCatBottom);
	auto width = Stack.Get(xoCatWidth);
	auto height = Stack.Get(xoCatHeight);
	if (!left.IsNull())	box.Left = reference.Left + ComputeDimension(reference.Width(), refWidthDefined, left.GetSize());
	if (!right.IsNull())	box.Right = reference.Right + ComputeDimension(reference.Width(), refWidthDefined, right.GetSize());
	if (!top.IsNull())	box.Top = reference.Top + ComputeDimension(reference.Height(), refHeightDefined, top.GetSize());
	if (!bottom.IsNull())	box.Bottom = reference.Bottom + ComputeDimension(reference.Height(), refHeightDefined, bottom.GetSize());
	if (!width.IsNull() && !left.IsNull() && right.IsNull()) box.Right = box.Left + ComputeDimension(reference.Width(), refWidthDefined, width.GetSize());
	if (!width.IsNull() && left.IsNull() && !right.IsNull()) box.Left = box.Right - ComputeDimension(reference.Width(), refWidthDefined, width.GetSize());
	if (!height.IsNull() && !top.IsNull() && bottom.IsNull()) box.Bottom = box.Top + ComputeDimension(reference.Height(), refHeightDefined, height.GetSize());
	if (!height.IsNull() && top.IsNull() && !bottom.IsNull()) box.Top = box.Bottom - ComputeDimension(reference.Height(), refHeightDefined, height.GetSize());
	return box;
}

void xoLayout::ComputeRelativeOffset(const NodeState& s, xoBox& box)
{
	XOTODO;
	//auto left = Stack.Get( xoCatLeft );
	//auto top = Stack.Get( xoCatTop );
	//if ( !left.IsNull() )		box.Left += ComputeDimension( s.ParentContentBox.Width(), s.ParentContentBoxHasWidth, left.GetSize() );
	//if ( !top.IsNull() )		box.Top += ComputeDimension( s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, top.GetSize() );
}

xoBox xoLayout::ComputeBox(xoBox container, bool widthDefined, bool heightDefined, xoStyleCategories cat)
{
	return ComputeBox(container, widthDefined, heightDefined, Stack.GetSizeQuad(cat));
}

xoBox xoLayout::ComputeBox(xoBox container, bool widthDefined, bool heightDefined, xoSizeQuad box)
{
	xoBox b;
	b.Left = ComputeDimension(container.Width(), widthDefined, box.Left);
	b.Right = ComputeDimension(container.Width(), widthDefined, box.Right);
	b.Top = ComputeDimension(container.Height(), heightDefined, box.Top);
	b.Bottom = ComputeDimension(container.Height(), heightDefined, box.Bottom);
	return b;
}
