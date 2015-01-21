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
	
	// These are thumbsuck numbers.
	// 100 is max expected tree depth.
	// 64 is related to size of LayoutOutput3, and number of expected objects
	// inside the vectors that store LayoutOutput3 inside RunNode3
	FHeap.Initialize(100, 64);		

	Fonts = xoGlobal()->FontStore->GetImmutableTable();
	SnapBoxes = xoGlobal()->SnapBoxes;
	SnapSubpixelHorzText = xoGlobal()->SnapSubpixelHorzText;
	EnableKerning = xoGlobal()->EnableKerning;

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
	in.ParentRNode = &root;
	in.RestartPoints = nullptr;

	LayoutOutput3 out;

	Boxer.BeginDocument();
	RunNode3(&Doc->Root, in, out);
	Boxer.EndDocument();
}

void xoLayout3::RunNode3(const xoDomNode* node, const LayoutInput3& in, LayoutOutput3& out)
{
	static int debug_num_run;
	debug_num_run++;

	xoBoxLayout3::NodeInput boxIn;

	xoStyleResolver::ResolveAndPush(Stack, node);

	xoBox margin = ComputeBox(in.ParentWidth, in.ParentHeight, xoCatMargin_Left);
	xoBox padding = ComputeBox(in.ParentWidth, in.ParentHeight, xoCatPadding_Left);
	xoBox border = ComputeBox(in.ParentWidth, in.ParentHeight, xoCatBorder_Left);
	xoPos contentWidth = ComputeDimension(in.ParentWidth, xoCatWidth);
	xoPos contentHeight = ComputeDimension(in.ParentHeight, xoCatHeight);

	if (SnapBoxes)
	{
		if (IsDefined(contentWidth))	contentWidth = xoPosRoundUp(contentWidth);
		if (IsDefined(contentHeight))	contentHeight = xoPosRoundUp(contentHeight);
	}

	xoRenderDomNode* rnode = new(Pool->AllocT<xoRenderDomNode>(false)) xoRenderDomNode(node->GetInternalID(), node->GetTag(), Pool);
	in.ParentRNode->Children += rnode;

	boxIn.InternalID = node->GetInternalID();
	boxIn.Tag = node->GetTag();
	boxIn.ContentWidth = contentWidth;
	boxIn.ContentHeight = contentHeight;
	boxIn.MarginBorderPadding = margin.PiecewiseSum(border).PiecewiseSum(padding);
	boxIn.NewFlowContext = Stack.Get(xoCatFlowContext).GetFlowContext() == xoFlowContextNew || node->GetTag() == xoTagBody; // Body MUST be a new flow context

	podvec<int32> myRestartPoints;

	LayoutInput3 childIn;
	childIn.ParentWidth = contentWidth;
	childIn.ParentHeight = contentHeight;
	childIn.ParentRNode = rnode;
	if (boxIn.NewFlowContext)
		childIn.RestartPoints = &myRestartPoints;
	else
		childIn.RestartPoints = in.RestartPoints;

	Boxer.BeginNode(boxIn);

	intp istart = 0;
	if (childIn.RestartPoints->size() != 0)
		istart = childIn.RestartPoints->rpop();

	// Remember that childOuts can be larger than node->ChildCount(), due to restarts.
	// childOuts contains a unique entry for every generated render-node.
	xoFixedVector<LayoutOutput3> childOuts(FHeap);

	for (intp i = istart; i < node->ChildCount(); i++)
	{
		LayoutOutput3 childOut;
		const xoDomEl* c = node->ChildByIndex(i);
		if (c->IsNode())
		{
			RunNode3(static_cast<const xoDomNode*>(c), childIn, childOut);
		}
		else
		{
			RunText3(static_cast<const xoDomText*>(c), childIn, childOut);
		}

		Boxer.SetBaseline(childOut.Baseline, (int) childOuts.Size());
		childOuts.Push(childOut);

		if (childIn.RestartPoints->size() != 0)
		{
			// Child is breaking out. Continue to break out until we hit a NewFlowContext
			if (boxIn.NewFlowContext)
			{
				// We are the final stop on a restart. So restart at our current child.
				i--;
				Boxer.Restart();
				// If all children are going to restart at zero, then delete the rnode that we already
				// created, because it will consist purely of empty husks. Unfortunately the empty children
				// end up as garbage memory in our render pool. One could probably reclaim the memory from
				// the pool in most cases, but I'm not convinced it's worth the effort.
				if (IsAllZeros(*childIn.RestartPoints))
					rnode->Children.Count--;
			}
			else
			{
				// We are just an intermediate node along the way
				childIn.RestartPoints->push((int32) i);
				break;
			}
		}
	}

	// I don't know yet how to think about having a restart initiated here. So far
	// I have only thought about the case where a restart is initiated from a word
	// emitted by a text object.
	xoBox marginBox;
	bool restart = Boxer.EndNode(marginBox) == xoBoxLayout3::FlowRestart;
	if (restart)
	{
		XOPANIC("Untested layout restart position");
		in.RestartPoints->push(0);
	}

	// Boxer doesn't know what our element's padding or margins are. The only thing it
	// emits is the margin-box for our element. We need to subtract the margin and
	// the padding in order to compute the content-box, which is what xoRenderDomNode needs.
	rnode->Pos = marginBox.ShrunkBy(boxIn.MarginBorderPadding);
	rnode->SetStyle(Stack);
	rnode->Style.BorderRadius = xoPosToReal(ComputeDimension(in.ParentWidth, xoCatBorderRadius));
	rnode->Style.BorderSize = border;
	rnode->Style.Padding = padding;

	// Apply alignment bindings
	if (childIn.ParentWidth == xoPosNULL)
		childIn.ParentWidth = rnode->Pos.Width();
	
	if (childIn.ParentHeight == xoPosNULL)
		childIn.ParentHeight = rnode->Pos.Height();

	// This tracks the line that we're on. The boxer keeps track of the last entity that got placed
	// on every line, and we use that information to figure out which line our child is on.
	int linebox_index = 0;
	auto linebox = Boxer.GetLineFromPreviousNode(linebox_index);
	xoPos myBaseline = linebox.InnerBaseline;
	for (intp i = 0; i < childOuts.Size(); i++)
	{
		while (i > linebox.LastChild)
			linebox = Boxer.GetLineFromPreviousNode(++linebox_index);

		if (childOuts[i].RNode != nullptr)
			PositionChildFromBindings(childIn, linebox.InnerBaseline, childOuts[i]);
	}

	if (myBaseline != xoPosNULL)
		out.Baseline = myBaseline + rnode->Pos.Top;	// we emit baseline in the coordinate system of our parent
	else
		out.Baseline = xoPosNULL;

	out.Binds = ComputeBinds();
	out.RNode = rnode;
	out.MarginBoxWidth = marginBox.Width();
	out.MarginBoxHeight = marginBox.Height();

	Stack.StackPop();
}

void xoLayout3::RunText3(const xoDomText* node, const LayoutInput3& in, LayoutOutput3& out)
{
	//XOTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 1\n", node.GetInternalID() );

	xoFontID fontID = Stack.Get(xoCatFontFamily).GetFont();

	xoStyleAttrib fontSizeAttrib = Stack.Get(xoCatFontSize);
	xoPos fontHeight = ComputeDimension(in.ParentHeight, fontSizeAttrib.GetSize());

	float fontSizePxUnrounded = xoPosToReal(fontHeight);

	// round font size to integer units
	uint8 fontSizePx = (uint8) xoRound(fontSizePxUnrounded);

	// Nothing prevents somebody from setting a font size to zero
	if (fontSizePx < 1)
		return;

	TempText.Node = node;
	TempText.RNode = in.ParentRNode;
	TempText.RNodeTxt = nullptr;
	TempText.FontWidthScale = 1.0f;
	TempText.IsSubPixel = xoGlobal()->EnableSubpixelText && fontSizePx <= xoGlobal()->MaxSubpixelGlyphSize;
	TempText.FontID = fontID;
	TempText.FontSizePx = fontSizePx;
	TempText.Color = Stack.Get(xoCatColor).GetColor();
	TempText.RestartPoints = in.RestartPoints;
	TempText.FontAscender = xoPosNULL;
	GenerateTextWords(TempText);

	// If we are restarting output, then it's possible that some characters in the
	// buffer were not flushed. So just wipe the buffer always.
	TempText.Chars.Clear();

	out.Baseline = TempText.FontAscender;
	// I can't think why you'd want to align text objects. Surely you'd rather align the containing elements?
	// Since all of our binding points are null, we don't need to populate our width and height, so we leave them zero.
	out.MarginBoxHeight = 0;
	out.MarginBoxWidth = 0;
	out.Binds = BindingSet{ xoHorizontalBindingNULL, xoHorizontalBindingNULL, xoVerticalBindingNULL, xoVerticalBindingNULL };
	out.RNode = TempText.RNodeTxt;
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
void xoLayout3::GenerateTextWords(TextRunState& ts)
{
	XOASSERTDEBUG(ts.Chars.Size() == 0);

	const char* txt = ts.Node->GetText();
	int32 txt_offset = 0;
	if (ts.RestartPoints->size() != 0)
	{
		txt_offset = ts.RestartPoints->rpop();
		txt += txt_offset;
		XOASSERT(ts.RestartPoints->size() == 0); // Text is a leaf node. The restart stack must be empty now.
	}

	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey(ts);
	const xoFont* font = Fonts.GetByFontID(ts.FontID);

	xoPos fontHeightRounded = xoRealToPos((float) ts.FontSizePx);
	xoPos charWidth_32 = xoRealx256ToPos(font->LinearHoriAdvance_Space_x256) * ts.FontSizePx;
	xoPos fontAscender = xoRealx256ToPos(font->Ascender_x256 * ts.FontSizePx);
	ts.FontAscender = fontAscender;

	if (SnapSubpixelHorzText)
		charWidth_32 = xoPosRound(charWidth_32);

	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos(ts.FontSizePx * font->LineHeight_x256);
	if (xoGlobal()->RoundLineHeights)
		lineHeight = xoPosRoundUp(lineHeight);

	if (strcmp(txt, "brown fox jumps") == 0)
		int abc = 123;

	ts.GlyphsNeeded = false;
	const xoGlyph* prevGlyph = nullptr;
	xoRenderDomText* rtxt = nullptr;
	xoPos rtxt_left = xoPosNULL;
	xoPos lastWordTop = xoPosNULL;
	int numCharsInQueue = 0;			// Number of characters in TextRunState::Chars that are waiting to be flushed to rtxt
	bool aborted = false;
	Chunk chunk;
	Chunker chunker(txt);
	while (!aborted && chunker.Next(chunk))
	{
		switch (chunk.Type)
		{
		case ChunkWord:
		{
			int32 chunkLen = chunk.End - chunk.Start;
			xoPos wordWidth = MeasureWord(txt, font, fontAscender, chunk, ts);

			if (ts.GlyphsNeeded)
				continue;

			if (strncmp(txt + chunk.Start, "jumps", 5) == 0)
				int abcd = 123;

			// output word
			xoRenderDomText* rtxt_new = nullptr;
			xoBoxLayout3::WordInput wordin;
			wordin.Width = wordWidth;
			wordin.Height = lineHeight;
			xoBox marginBox;
			if (Boxer.AddWord(wordin, marginBox) == xoBoxLayout3::FlowRestart)
			{
				aborted = true;
				ts.RestartPoints->push(chunk.Start + txt_offset);
				break;
			}

			if (rtxt == nullptr || marginBox.Top != lastWordTop)
			{
				// We need a new output object
				xoRenderDomText* rtxt_new = new(Pool->AllocT<xoRenderDomText>(false)) xoRenderDomText(ts.Node->GetInternalID(), Pool);
				ts.RNode->Children += rtxt_new;
				if (rtxt != nullptr)
				{
					// retire previous text object - which is all characters in the queue, except for the most recent word
					FinishTextRNode(ts, rtxt, numCharsInQueue);
				}
				rtxt_new->Pos = marginBox;
				rtxt = rtxt_new;
				rtxt_left = marginBox.Left;
				numCharsInQueue = chunkLen;
			}
			else
			{
				// another word on existing line
				numCharsInQueue += chunkLen;
				OffsetTextHorz(ts, marginBox.Left - rtxt_left, chunkLen);
			}
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
	ts.RNodeTxt = rtxt;
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
		if (EnableKerning && prevGlyph)
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

xoPoint xoLayout3::PositionChildFromBindings(const LayoutInput3& cin, xoPos parentBaseline, const LayoutOutput3& cout)
{
	xoPoint child, parent;
	child.X = HBindOffset(cout.Binds.HChild, cout.MarginBoxWidth);
	child.Y = VBindOffset(cout.Binds.VChild, cout.Baseline, cout.MarginBoxHeight);
	parent.X = HBindOffset(cout.Binds.HParent, cin.ParentWidth);
	parent.Y = VBindOffset(cout.Binds.VParent, parentBaseline, cin.ParentHeight);
	xoPoint offset(parent.X - child.X, parent.Y - child.Y);
	cout.RNode->Pos.Offset(offset);
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
	if (hcenter != xoHorizontalBindingNULL)		{ binds.HChild = xoHorizontalBindingCenter; binds.HParent = hcenter; }
	if (right != xoHorizontalBindingNULL)		{ binds.HChild = xoHorizontalBindingRight; binds.HParent = right; }

	if (top != xoVerticalBindingNULL)			{ binds.VChild = xoVerticalBindingTop; binds.VParent = top; }
	if (vcenter != xoVerticalBindingNULL)		{ binds.VChild = xoVerticalBindingCenter; binds.VParent = vcenter; }
	if (bottom != xoVerticalBindingNULL)		{ binds.VChild = xoVerticalBindingBottom; binds.VParent = bottom; }
	if (baseline != xoVerticalBindingNULL)		{ binds.VChild = xoVerticalBindingBaseline; binds.VParent = baseline; }

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

bool xoLayout3::IsAllZeros(const podvec<int32>& list)
{
	for (intp i = 0; i < list.size(); i++)
	{
		if (list[i] != 0)
			return false;
	}
	return true;
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
