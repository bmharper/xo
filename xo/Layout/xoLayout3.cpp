#include "pch.h"
#include "xoLayout3.h"
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
void xoLayout3::Layout(const xoDoc& doc, xoRenderDomNode& root, xoPool* pool)
{
	Doc = &doc;
	Pool = pool;
	Boxer.Pool = pool;
	Stack.Initialize(Doc, Pool);
	ChildOutStack.Init(1024 * 1024);
	LineBoxStack.Init(1024 * 1024);
	Fonts = xoGlobal()->FontStore->GetImmutableTable();
	SnapBoxes = xoGlobal()->SnapBoxes;
	SnapSubpixelHorzText = xoGlobal()->SnapSubpixelHorzText;

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

void xoLayout3::RenderGlyphsNeeded()
{
	for (auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++)
		xoGlobal()->GlyphCache->RenderGlyph(*it);
	GlyphsNeeded.clear();
}

void xoLayout3::LayoutInternal(xoRenderDomNode& root)
{
	PtToPixel = 1.0;	// TODO
	EpToPixel = xoGlobal()->EpToPixel;

	XOTRACE_LAYOUT_VERBOSE("Layout 1\n");

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	XOTRACE_LAYOUT_VERBOSE("Layout 2\n");

	LayoutInput3 in;
	in.ParentWidth = xoIntToPos(Doc->UI.GetViewportWidth());
	in.ParentHeight = xoIntToPos(Doc->UI.GetViewportHeight());

	Boxer.BeginDocument(&root);
	RunNode3(&Doc->Root, in);
	Boxer.EndDocument();
}

void xoLayout3::RunNode3(const xoDomNode* node, const LayoutInput3& in)
{
	xoBoxLayout3::NodeInput boxIn;

	xoStyleResolver::ResolveAndPush(Stack, node);
	//rnode->SetStyle( Stack );

	xoPos contentWidth = ComputeDimension(in.ParentWidth, xoCatWidth);
	xoPos contentHeight = ComputeDimension(in.ParentHeight, xoCatHeight);

	boxIn.InternalID = node->GetInternalID();
	boxIn.Tag = node->GetTag();
	boxIn.ContentWidth = contentWidth;
	boxIn.ContentHeight = contentHeight;
	boxIn.NewFlowContext = Stack.Get(xoCatFlowContext).GetFlowContext() == xoFlowContextNew;

	LayoutInput3 childIn;
	childIn.ParentWidth = contentWidth;
	childIn.ParentHeight = contentHeight;

	Boxer.BeginNode(boxIn);

	for (intp i = 0; i < node->ChildCount(); i++)
	{
		const xoDomEl* c = node->ChildByIndex(i);
		if (c->IsNode())
		{
			RunNode3(static_cast<const xoDomNode*>(c), childIn);
		}
		else
		{
			RunText3(static_cast<const xoDomText*>(c), childIn);
		}
	}
	xoRenderDomNode* rnode;
	xoBox marginBox;
	Boxer.EndNode(rnode, marginBox);
	// Boxer doesn't know what our element's padding or margins are. The only thing it
	// emits is the margin-box for our element. We need to subtract the margin and
	// the padding in order to compute the content-box, which is what xoRenderDomNode needs.
	xoBox margin = ComputeBox(in.ParentWidth, in.ParentHeight, xoCatMargin_Left);
	xoBox padding = ComputeBox(in.ParentWidth, in.ParentHeight, xoCatPadding_Left);
	rnode->Pos = marginBox.ShrunkBy(margin).ShrunkBy(padding);
	rnode->SetStyle(Stack);
}

void xoLayout3::RunText3(const xoDomText* node, const LayoutInput3& in)
{
	//XOTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 1\n", node.GetInternalID() );
	//rnode->InternalID = node.GetInternalID();
	//rnode->SetStyle( Stack );

	//XOTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 2\n", node.GetInternalID() );

	//rnode->FontID = Stack.Get( xoCatFontFamily ).GetFont();
	xoFontID fontID = Stack.Get(xoCatFontFamily).GetFont();

	xoStyleAttrib fontSizeAttrib = Stack.Get(xoCatFontSize);
	xoPos fontHeight = ComputeDimension(in.ParentHeight, fontSizeAttrib.GetSize());

	float fontSizePxUnrounded = xoPosToReal(fontHeight);

	// round font size to integer units
	//rnode->FontSizePx = (uint8) xoRound( fontSizePxUnrounded );
	uint8 fontSizePx = (uint8) xoRound(fontSizePxUnrounded);

	//out.NodeWidth = 0;
	//out.NodeHeight = 0;
	//out.NodeBaseline = 0;

	// Nothing prevents somebody from setting a font size to zero
	//if ( rnode->FontSizePx < 1 )
	if (fontSizePx < 1)
		return;

	bool subPixel = xoGlobal()->EnableSubpixelText && fontSizePx <= xoGlobal()->MaxSubpixelGlyphSize;
	//if ( subPixel )
	//	rnode->Flags |= xoRenderDomText::FlagSubPixelGlyphs;

	//xoBoxLayout3::NodeInput bnode;
	//bnode.NewFlowContext = false;
	//bnode.ContentHeight = xoPosNULL;
	//bnode.ContentWidth = xoPosNULL;
	//bnode.InternalID = node->GetInternalID();
	//bnode.Margin = xoBox(0,0,0,0);
	//bnode.Padding = xoBox(0,0,0,0);
	//bnode.Tag = xoTagText;
	//Boxer.BeginNode( bnode );

	TempText.Node = node;
	//TempText.RNode = rnode;
	TempText.Words.clear_noalloc();
	TempText.GlyphCount = 0;
	TempText.FontWidthScale = 1.0f;
	TempText.IsSubPixel = subPixel;
	TempText.FontID = fontID;
	TempText.FontSizePx = fontSizePx;
	TempText.Color = Stack.Get(xoCatColor).GetColor();
	GenerateTextWords(TempText);

	//Boxer.EndNode();
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
void xoLayout3::GenerateTextOutput(const LayoutInput& in, LayoutOutput& out, TextRunState& ts)
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts.RNode);
	const xoFont* font = Fonts.GetByFontID(ts.RNode->FontID);

	xoPos fontHeightRounded = xoRealToPos(ts.RNode->FontSizePx);
	xoPos fontAscender = xoRealx256ToPos(font->Ascender_x256 * ts.RNode->FontSizePx);

	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos(ts.RNode->FontSizePx * font->LineHeight_x256);
	if (xoGlobal()->RoundLineHeights)
		lineHeight = xoPosRoundUp(lineHeight);

	int fontHeightPx = ts.RNode->FontSizePx;
	ts.RNode->Text.reserve(ts.GlyphCount);
	bool parentHasWidth = IsDefined(in.ParentWidth);
	bool enableKerning = xoGlobal()->EnableKerning;

	xoPos posX = 0;
	xoPos posMaxX = 0;
	xoPos posMaxY = 0;
	xoPos baseline = fontAscender;
	out.NodeBaseline = baseline;

	for (intp iword = 0; iword < ts.Words.size(); iword++)
	{
		const Word& word = ts.Words[iword];
		bool isSpace = word.Length() == 1 && txt[word.Start] == 32;
		bool isNewline = word.Length() == 1 && txt[word.Start] == '\n';
		bool over = parentHasWidth ? posX + word.Width > in.ParentWidth : false;
		if (over)
		{
			bool futile = posX == 0;
			if (!futile)
			{
				//NextLine( s );
				baseline += lineHeight;
				posX = 0;
				// If the line break was performed for a space, then treat that space as "done"
				if (isSpace)
					continue;
			}
		}

		if (isSpace)
		{
			posX += xoRealx256ToPos(font->LinearHoriAdvance_Space_x256) * fontHeightPx;
		}
		else if (isNewline)
		{
			//NextLine( s );
			baseline += lineHeight;
			posX = 0;
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
					posX += kerning;
				}
				ts.RNode->Text.Count++;
				xoRenderCharEl& rtxt = ts.RNode->Text.back();
				rtxt.Char = key.Char;
				rtxt.X = posX + xoRealx256ToPos(glyph->MetricLeftx256);
				rtxt.Y = baseline - xoRealToPos(glyph->MetricTop);			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
				posX += HoriAdvance(glyph, ts);
				posMaxX = xoMax(posMaxX, posX);
				prevGlyph = glyph;
			}
		}
		posMaxY = xoMax(posMaxY, baseline - fontAscender + lineHeight);
	}

	out.NodeWidth = posMaxX;
	out.NodeHeight = posMaxY;
}

/*
Calling FinishTextRNode:
The order of events here can be a little bit confusing. We need to
do it this way to ensure that we only write out the characters into
the xoRenderDomText element once. The simpler approach would be to
append characters to xoRenderDomText after every word, but that
appending involves growing a vector, so plenty of memory reallocs.
Instead, we queue up a string of characters and write them all
out at once, when we either detect a new line, or when we are done
with the entire text object.
Having this tight coupling between xoLayout3 and xoBoxLayout is
unfortunate. Perhaps if all the dust has settled, then it might
be worth it to break that coupling, to the degree that xoLayout3
doesn't know where its word boxes are going - it just dumps them
to xoBoxLayout and forgets about them.
This would mean that xoBoxLayout is responsible for adjusting
the positions of the glyphs inside an xoRenderDomText object.
*/
void xoLayout3::GenerateTextWords(TextRunState& ts)
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts);
	const xoFont* font = Fonts.GetByFontID(ts.FontID);

	xoPos fontHeightRounded = xoRealToPos((float) ts.FontSizePx);
	xoPos fontAscender = xoRealx256ToPos(font->Ascender_x256 * ts.FontSizePx);
	xoPos charWidth_32 = xoRealx256ToPos(font->LinearHoriAdvance_Space_x256) * ts.FontSizePx;

	if (SnapSubpixelHorzText)
		charWidth_32 = xoPosRound(charWidth_32);

	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos(ts.FontSizePx * font->LineHeight_x256);
	if (xoGlobal()->RoundLineHeights)
		lineHeight = xoPosRoundUp(lineHeight);

	ts.GlyphsNeeded = false;
	xoPos baseline = fontAscender;
	const xoGlyph* prevGlyph = nullptr;
	xoRenderDomText* rtxt = nullptr;
	int numCharsInQueue = 0;			// Number of characters in TextRunState::Chars that are waiting to be flushed to rtxt
	Chunk chunk;
	Chunker chunker(txt);
	while (chunker.Next(chunk))
	{
		switch (chunk.Type)
		{
		case ChunkWord:
		{
			int32 chunkLen = chunk.End - chunk.Start;
			xoPos wordWidth = MeasureWord(txt, font, fontAscender, chunk, ts);

			if (ts.GlyphsNeeded)
				continue;

			// output word
			xoRenderDomText* rtxt_new = nullptr;
			xoBoxLayout3::WordInput wordin;
			wordin.Width = wordWidth;
			wordin.Height = lineHeight; // unsure
			xoPos posX = 0;
			Boxer.AddWord(wordin, rtxt_new, posX);

			if (rtxt_new != nullptr && rtxt == nullptr)
			{
				// first output
				rtxt = rtxt_new;
				numCharsInQueue = chunkLen;
			}
			else if (rtxt_new != rtxt && rtxt != nullptr)
			{
				// first word on new line. retire the old line, and start a new one.
				FinishTextRNode(ts, rtxt, numCharsInQueue);
				rtxt = rtxt_new;
				numCharsInQueue = chunkLen;
			}
			else if (rtxt_new != nullptr && rtxt != nullptr && rtxt_new == rtxt)
			{
				// another word on existing line
				numCharsInQueue += chunkLen;
			}
			OffsetTextHorz(ts, posX, chunkLen);
		}
		break;
		case ChunkSpace:
			Boxer.AddSpace(charWidth_32);
			break;
		case ChunkLineBreak:
			Boxer.AddLinebreak();
			break;
		}
	}
	// the end
	if (rtxt != nullptr)
		FinishTextRNode(ts, rtxt, numCharsInQueue);
}

void xoLayout3::FinishTextRNode(TextRunState& ts, xoRenderDomText* rnode, intp numChars)
{
	rnode->FontID = ts.FontID;
	rnode->Color = ts.Color;
	rnode->FontSizePx = ts.FontSizePx;
	if (ts.IsSubPixel)
		rnode->Flags |= xoRenderDomText::FlagSubPixelGlyphs;

	XOASSERT(ts.Chars.Size() >= numChars);

	rnode->Text.resize(numChars);
	for (intp i = 0; i < numChars; i++)
		rnode->Text[i] = ts.Chars.PopTail();
}

void xoLayout3::OffsetTextHorz(TextRunState& ts, xoPos offsetHorz, intp numChars)
{
	for (int i = 0; i < numChars; i++)
		ts.Chars.FromHead(i).X += offsetHorz;
}

// While measuring the length of the word, we are also recording its character placements.
// All characters go into a queue, which gets flushed whenever we flow onto a new line.
// Returns the width of the word
xoPos xoLayout3::MeasureWord(const char* txt, const xoFont* font, xoPos fontAscender, Chunk chunk, TextRunState& ts)
{
	// I find it easier to understand when referring to this value as "baseline" instead of "ascender"
	xoPos baseline = fontAscender;

	xoPos posX = 0;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts);
	const xoGlyph* prevGlyph = nullptr;

	for (int32 i = chunk.Start; i < chunk.End; i++)
	{
		key.Char = txt[i];
		const xoGlyph* glyph = xoGlobal()->GlyphCache->GetGlyph(key);
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
			prevGlyph = nullptr;
		}
		if (xoGlobal()->EnableKerning && prevGlyph)
		{
			// Multithreading hazard here. I'm not sure whether FT_Get_Kerning is thread safe.
			// Also, I have stepped inside there and I see it does a binary search. We might
			// be better off caching the kerning for frequent pairs of glyphs in a hash table.
			FT_Vector kern;
			FT_Get_Kerning(font->FTFace, prevGlyph->FTGlyphIndex, glyph->FTGlyphIndex, FT_KERNING_UNSCALED, &kern);
			xoPos kerning = ((kern.x * ts.FontSizePx) << xoPosShift) / font->FTFace->units_per_EM;
			posX += kerning;
		}

		xoRenderCharEl& rtxt = ts.Chars.PushHead();
		rtxt.Char = key.Char;
		rtxt.X = posX + xoRealx256ToPos(glyph->MetricLeftx256);
		rtxt.Y = baseline - xoRealToPos(glyph->MetricTop);			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
		// For determining the word width, one might want to not use the horizontal advance for the very last glyph, but instead
		// use the glyph's exact width. The difference would be tiny, and it may even be annoying, because you would end up with
		// no padding on the right side of a word.
		posX += HoriAdvance(glyph, ts);
		prevGlyph = glyph;
	}
	return posX;
}

xoPoint xoLayout3::PositionChildFromBindings(const LayoutInput& cin, const LayoutOutput& cout, xoRenderDomEl* rchild)
{
	xoPoint child, parent;
	child.X = HBindOffset(cout.Binds.HChild, cout.NodeWidth);
	child.Y = VBindOffset(cout.Binds.VChild, cout.NodeBaseline, cout.NodeHeight);
	parent.X = HBindOffset(cout.Binds.HParent, cin.ParentWidth);
	parent.Y = VBindOffset(cout.Binds.VParent, cin.OuterBaseline, cin.ParentHeight);
	xoPoint offset(parent.X - child.X, parent.Y - child.Y);
	rchild->Pos.Offset(offset);
	return offset;
}

xoPos xoLayout3::ComputeDimension(xoPos container, xoStyleCategories cat)
{
	return ComputeDimension(container, Stack.Get(cat).GetSize());
}

xoPos xoLayout3::ComputeDimension(xoPos container, xoSize size)
{
	switch (size.Type)
	{
	case xoSize::NONE: return xoPosNULL;
	case xoSize::PX: return xoRealToPos(size.Val);
	case xoSize::PT: return xoRealToPos(size.Val * PtToPixel);
	case xoSize::EP: return xoRealToPos(size.Val * EpToPixel);
	case xoSize::PERCENT:
		if (container == xoPosNULL)
			return xoPosNULL;
		else
			return xoPos(xoRound((float) container * (size.Val * 0.01f)));   // this might be sloppy floating point. Small rational percentages like 25% (1/4) ought to be preserved precisely.
	default: XOPANIC("Unrecognized size type"); return 0;
	}
}

xoBox xoLayout3::ComputeBox(xoPos containerWidth, xoPos containerHeight, xoStyleCategories cat)
{
	return ComputeBox(containerWidth, containerHeight, Stack.GetBox(cat));
}

xoBox xoLayout3::ComputeBox(xoPos containerWidth, xoPos containerHeight, xoStyleBox box)
{
	xoBox b;
	b.Left = ComputeDimension(containerWidth, box.Left);
	b.Right = ComputeDimension(containerWidth, box.Right);
	b.Top = ComputeDimension(containerHeight, box.Top);
	b.Bottom = ComputeDimension(containerHeight, box.Bottom);
	return b;
}

xoLayout3::BindingSet xoLayout3::ComputeBinds()
{
	xoHorizontalBindings left = Stack.Get(xoCatLeft).GetHorizontalBinding();
	xoHorizontalBindings hcenter = Stack.Get(xoCatHCenter).GetHorizontalBinding();
	xoHorizontalBindings right = Stack.Get(xoCatRight).GetHorizontalBinding();

	xoVerticalBindings top = Stack.Get(xoCatTop).GetVerticalBinding();
	xoVerticalBindings vcenter = Stack.Get(xoCatVCenter).GetVerticalBinding();
	xoVerticalBindings bottom = Stack.Get(xoCatBottom).GetVerticalBinding();
	xoVerticalBindings baseline = Stack.Get(xoCatBaseline).GetVerticalBinding();

	BindingSet binds = {xoHorizontalBindingNULL, xoHorizontalBindingNULL, xoVerticalBindingNULL, xoVerticalBindingNULL};

	if (left != xoHorizontalBindingNULL)		{ binds.HChild = xoHorizontalBindingLeft; binds.HParent = left; }
	if (hcenter != xoHorizontalBindingNULL)	{ binds.HChild = xoHorizontalBindingCenter; binds.HParent = hcenter; }
	if (right != xoHorizontalBindingNULL)		{ binds.HChild = xoHorizontalBindingRight; binds.HParent = right; }

	if (top != xoVerticalBindingNULL)			{ binds.VChild = xoVerticalBindingTop; binds.VParent = top; }
	if (vcenter != xoVerticalBindingNULL)		{ binds.VChild = xoVerticalBindingCenter; binds.VParent = vcenter; }
	if (bottom != xoVerticalBindingNULL)		{ binds.VChild = xoVerticalBindingBottom; binds.VParent = bottom; }
	if (baseline != xoVerticalBindingNULL)	{ binds.VChild = xoVerticalBindingBaseline; binds.VParent = baseline; }

	return binds;
}

xoPos xoLayout3::HoriAdvance(const xoGlyph* glyph, const TextRunState& ts)
{
	if (SnapSubpixelHorzText)
		return xoIntToPos(glyph->MetricHoriAdvance);
	else
		return xoRealToPos(glyph->MetricLinearHoriAdvance * ts.FontWidthScale);
}

xoPos xoLayout3::HBindOffset(xoHorizontalBindings bind, xoPos width)
{
	switch (bind)
	{
	case xoHorizontalBindingNULL:
	case xoHorizontalBindingLeft:	return 0;
	case xoHorizontalBindingCenter:	return width / 2;
	case xoHorizontalBindingRight:	return width;
	default:
		XOASSERTDEBUG(false);
		return 0;
	}
}

xoPos xoLayout3::VBindOffset(xoVerticalBindings bind, xoPos baseline, xoPos height)
{
	switch (bind)
	{
	case xoVerticalBindingNULL:
	case xoVerticalBindingTop:		return 0;
	case xoVerticalBindingCenter:	return height / 2;
	case xoVerticalBindingBottom:	return height;
	case xoVerticalBindingBaseline:
		if (IsDefined(baseline))
			return baseline;
		else
		{
			XOTRACE_LAYOUT_WARNING("Undefined baseline used in alignment\n");
			return height;
		}
	default:
		XOASSERTDEBUG(false);
		return 0;
	}
}

bool xoLayout3::IsSpace(int ch)
{
	return ch == 32;
}

bool xoLayout3::IsLinebreak(int ch)
{
	return ch == '\r' || ch == '\n';
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey(xoRenderDomText* rnode)
{
	uint8 glyphFlags = rnode->IsSubPixel() ? xoGlyphFlag_SubPixel_RGB : 0;
	return xoGlyphCacheKey(rnode->FontID, 0, rnode->FontSizePx, glyphFlags);
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey(const TextRunState& ts)
{
	return MakeGlyphCacheKey(ts.IsSubPixel, ts.FontID, ts.FontSizePx);
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey(bool isSubPixel, xoFontID fontID, int fontSizePx)
{
	uint8 glyphFlags = isSubPixel ? xoGlyphFlag_SubPixel_RGB : 0;
	return xoGlyphCacheKey(fontID, 0, fontSizePx, glyphFlags);
}

void xoLayout3::FlowNewline(FlowState& flow)
{
	flow.PosMinor = 0;
	flow.PosMajor = flow.MajorMax;
	flow.NumLines++;
}

bool xoLayout3::FlowBreakBefore(const LayoutOutput& cout, FlowState& flow)
{
	if (cout.GetBreak() == xoBreakBefore)
	{
		FlowNewline(flow);
		return true;
	}
	return false;
}

xoPoint xoLayout3::FlowRun(const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, xoRenderDomEl* rendEl)
{
	if (IsDefined(cin.ParentWidth))
	{
		bool over = flow.PosMinor + cout.NodeWidth > cin.ParentWidth;
		bool futile = flow.PosMinor == 0;
		if (over && !futile)
			FlowNewline(flow);
	}
	xoPoint offset(flow.PosMinor, flow.PosMajor);
	rendEl->Pos.Offset(offset.X, offset.Y);
	flow.PosMinor += cout.NodeWidth;
	flow.MajorMax = xoMax(flow.MajorMax, flow.PosMajor + cout.NodeHeight);

	if (cout.GetBreak() == xoBreakAfter)
		FlowNewline(flow);

	return offset;
}

xoLayout3::Chunker::Chunker(const char* txt) :
	Txt(txt),
	Pos(0)
{
}

bool xoLayout3::Chunker::Next(Chunk& c)
{
	if (Txt[Pos] == 0)
		return false;

	char first = Txt[Pos];
	c.Start = Pos;
	switch (first)
	{
	case 9:
	case 32:
		c.Type = ChunkSpace;
		for (; Txt[Pos] == first; Pos++) {}
		break;
	case '\r':
		c.Type = ChunkLineBreak;
		if (Txt[Pos] == '\n')
			Pos += 2;
		else
			Pos += 1;
		break;
	case '\n':
		c.Type = ChunkLineBreak;
		Pos++;
		break;
	default:
		c.Type = ChunkWord;
		while (true)
		{
			Pos++;
			char ch = Txt[Pos];
			if (ch == 0 || ch == 9 || ch == 32 || ch == '\r' || ch == '\n')
				break;
		}
	}
	c.End = Pos;
	return true;
}
