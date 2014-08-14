#include "pch.h"
#include "nuLayout2.h"
#include "nuDoc.h"
#include "Dom/nuDomNode.h"
#include "Dom/nuDomText.h"
#include "Render/nuRenderDomEl.h"
#include "Render/nuStyleResolve.h"
#include "Text/nuFontStore.h"
#include "Text/nuGlyphCache.h"

/* This is called serially.

Why do we perform layout in multiple passes, loading all missing glyphs at the end of each pass?
The reason is because we eventually want to be able to parallelize layout.

Missing glyphs are a once-off cost (ie once per application instance),
so it's not worth trying to use a mutable glyph cache.

*/
void nuLayout2::Layout( const nuDoc& doc, u32 docWidth, u32 docHeight, nuRenderDomNode& root, nuPool* pool )
{
	Doc = &doc;
	DocWidth = docWidth;
	DocHeight = docHeight;
	Pool = pool;
	Stack.Initialize( Doc, Pool );
	ChildOutStack.Init( 1024 * 1024 );
	LineBoxStack.Init( 1024 * 1024 );
	Fonts = nuGlobal()->FontStore->GetImmutableTable();
	SnapBoxes = nuGlobal()->SnapBoxes;
	SnapSubpixelHorzText = nuGlobal()->SnapSubpixelHorzText;

	while ( true )
	{
		LayoutInternal( root );

		if ( GlyphsNeeded.size() == 0 )
		{
			NUTRACE_LAYOUT_VERBOSE( "Layout done\n" );
			break;
		}
		else
		{
			NUTRACE_LAYOUT_VERBOSE( "Layout done (but need another pass for missing glyphs)\n" );
			RenderGlyphsNeeded();
		}
	}
}

void nuLayout2::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		nuGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

void nuLayout2::LayoutInternal( nuRenderDomNode& root )
{
	PtToPixel = 1.0;	// TODO
	EpToPixel = nuGlobal()->EpToPixel;

	NUTRACE_LAYOUT_VERBOSE( "Layout 1\n" );

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	NUTRACE_LAYOUT_VERBOSE( "Layout 2\n" );

	LayoutInput in;
	in.ParentWidth = nuIntToPos( DocWidth );
	in.ParentHeight = nuIntToPos( DocHeight );
	in.OuterBaseline = nuPosNULL;

	LayoutOutput out;

	NUTRACE_LAYOUT_VERBOSE( "Layout 3 DocBox = %d,%d,%d,%d\n", s.ParentContentBox.Left, s.ParentContentBox.Top, s.ParentContentBox.Right, s.ParentContentBox.Bottom );

	RunNode( Doc->Root, in, out, &root );
}

void nuLayout2::RunNode( const nuDomNode& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomNode* rnode )
{
	NUTRACE_LAYOUT_VERBOSE( "Layout (%d) Run 1\n", node.GetInternalID() );
	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );

	NUTRACE_LAYOUT_VERBOSE( "Layout (%d) Run 2\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();

	nuBoxSizeType boxSizing = Stack.Get( nuCatBoxSizing ).GetBoxSizing();
	nuPos borderRadius = ComputeDimension( 0, nuCatBorderRadius );
	nuPos contentWidth = ComputeDimension( in.ParentWidth, nuCatWidth );
	nuPos contentHeight = ComputeDimension( in.ParentHeight, nuCatHeight );
	nuBox margin = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatMargin_Left );		// it may be wise to disallow percentage sizing here
	nuBox padding = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatPadding_Left );	// same here
	nuBox border = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatBorder_Left );		// and here
	
	// It might make for less arithmetic if we work with marginBoxWidth and marginBoxHeight instead of contentBoxWidth and contentBoxHeight. We'll see.

	// This box holds the offsets from the 4 sides of our origin, to our content box. (Our origin is our parent's content box, but since it's relative here, it starts at 0,0)
	nuBox toContent;
	toContent.Left = margin.Left + border.Left + padding.Left;
	toContent.Right = margin.Right + border.Right + padding.Right;
	toContent.Top = margin.Top + border.Top + padding.Top;
	toContent.Bottom = margin.Bottom + border.Bottom + padding.Bottom;

	if ( boxSizing == nuBoxSizeContent ) {}
	else if ( boxSizing == nuBoxSizeBorder )
	{
		if ( IsDefined(contentWidth) )	contentWidth -= border.Left + border.Right + padding.Left + padding.Right;
		if ( IsDefined(contentHeight) )	contentHeight -= border.Top + border.Bottom + padding.Top + padding.Bottom;
	}
	else if ( boxSizing == nuBoxSizeMargin )
	{
		if ( IsDefined(contentWidth) )	contentWidth -= margin.Left + margin.Right + border.Left + border.Right + padding.Left + padding.Right;
		if ( IsDefined(contentHeight) )	contentHeight -= margin.Top + margin.Bottom + border.Top + border.Bottom + padding.Top + padding.Bottom;
	}

	if ( SnapBoxes )
	{
		if ( IsDefined(contentWidth) )	contentWidth = nuPosRoundUp( contentWidth );
		if ( IsDefined(contentHeight) )	contentHeight = nuPosRoundUp( contentHeight );
	}

	nuPoint borderToContent( border.Left + padding.Left, border.Top + padding.Top );

	nuPos autoWidth = 0;
	nuPos autoHeight = 0;
	nuPos outerBaseline = IsDefined(in.OuterBaseline) ? in.OuterBaseline - toContent.Top : nuPosNULL;
	//nuPos innerBaseline = nuPosNULL;
	//int innerBaselineDefinedBy = -1;

	// If we don't know our width and height yet then we need to delay bindings until our first pass is done
	// The buffer size of 16 here is thumbsuck. One can't make it too big, because this is a recursive function.
	// -- I first tried to have an optimized case for binding during first pass, but I have given up on that.
	// -- One could revisit it once the design is nailed down.
	//StackBufferT<LayoutOutput, 16> outs;
	//outs.Init( (int) node.ChildCount() );
	nuLifoVector<LayoutOutput> outs( ChildOutStack );
	outs.AddN( node.ChildCount() );
	nuLifoVector<LineBox> lineBoxes( LineBoxStack );
	lineBoxes.Push( LineBox::Make( nuPosNULL, -1, INT32MAX ) );

	FlowState flow;
	flow.PosMajor = 0;
	flow.PosMinor = 0;
	flow.MajorMax = 0;
	flow.NumLines = 0;

	for ( intp i = 0; i < node.ChildCount(); i++ )
	{
		const nuDomEl* c = node.ChildByIndex( i );
		LayoutInput cin;
		LayoutOutput cout;
		cin.OuterBaseline = IsDefined(outerBaseline) ? outerBaseline : lineBoxes.Back().InnerBaseline;
		cin.ParentWidth = contentWidth;
		cin.ParentHeight = contentHeight;
		nuPoint offset(0,0);
		int nlines = flow.NumLines;
		bool breakBefore = false;
		if ( c->GetTag() == nuTagText )
		{
			nuRenderDomText* rchildTxt = new (Pool->AllocT<nuRenderDomText>(false)) nuRenderDomText( c->GetInternalID(), Pool );
			rnode->Children += rchildTxt;
			RunText( *static_cast<const nuDomText*>(c), cin, cout, rchildTxt );
			breakBefore = FlowBreakBefore( cout, flow );
			offset += FlowRun( cin, cout, flow, rchildTxt );
			// Text elements cannot choose their layout. They are forced to start in the top-left of their parent, and perform text layout inside that space.
		}
		else
		{
			nuRenderDomNode* rchildNode = new (Pool->AllocT<nuRenderDomNode>(false)) nuRenderDomNode( c->GetInternalID(), c->GetTag(), Pool );
			rnode->Children += rchildNode;
			RunNode( *static_cast<const nuDomNode*>(c), cin, cout, rchildNode );
			breakBefore = FlowBreakBefore( cout, flow );
			offset += FlowRun( cin, cout, flow, rchildNode );
		}
		outs[i] = cout;
		if ( flow.NumLines != nlines && breakBefore )
		{
			// Create a new linebox BEFORE adding this child's state
			lineBoxes.Back().LastChild = int(i - 1);
			lineBoxes += LineBox::Make( nuPosNULL, -1, INT32MAX );
		}
		if ( IsNull(lineBoxes.Back().InnerBaseline) && IsDefined(cout.NodeBaseline) )
		{
			lineBoxes.Back().InnerBaseline = cout.NodeBaseline + offset.Y;
			lineBoxes.Back().InnerBaselineDefinedBy = (int) i;
		}
		if ( flow.NumLines != nlines && !breakBefore )
		{
			// Create a new linebox AFTER adding this child's state
			lineBoxes.Back().LastChild = (int) i;
			lineBoxes += LineBox::Make( nuPosNULL, -1, INT32MAX );
		}
		autoWidth = nuMax( autoWidth, flow.PosMinor );
		autoHeight = nuMax( autoHeight, flow.MajorMax );
	}

	if ( IsNull(contentWidth) ) contentWidth = SnapBoxes ? nuPosRoundUp(autoWidth) : autoWidth;
	if ( IsNull(contentHeight) ) contentHeight = SnapBoxes ?  nuPosRoundUp(autoHeight) : autoHeight;

	// Apply bindings
	{
		int iLineBox = 0;
		LayoutInput cin;
		cin.OuterBaseline = IsDefined(outerBaseline) ? outerBaseline : lineBoxes[iLineBox].InnerBaseline;
		cin.ParentWidth = contentWidth;
		cin.ParentHeight = contentHeight;
		for ( intp i = 0; i < node.ChildCount(); i++ )
		{
			const nuDomEl* c = node.ChildByIndex( i );
			nuPoint offset(0,0);
			if ( c->GetTag() != nuTagText )
				offset = PositionChildFromBindings( cin, outs[i], rnode->Children[i] );
			if ( i == lineBoxes[iLineBox].InnerBaselineDefinedBy )
				lineBoxes[iLineBox].InnerBaseline += offset.Y;
			if ( lineBoxes[iLineBox].LastChild == i )
				iLineBox++;
		}
	}

	rnode->Pos = nuBox( 0, 0, contentWidth, contentHeight ).OffsetBy( toContent.Left, toContent.Top );
	rnode->Style.BackgroundColor = Stack.Get( nuCatBackground ).GetColor();
	rnode->Style.BorderRadius = nuPosToReal( borderRadius );
	rnode->Style.BorderSize = border;
	rnode->Style.Padding = padding;
	rnode->Style.BorderColor = Stack.Get( nuCatBorderColor_Left ).GetColor();

	out.NodeBaseline = IsDefined(lineBoxes[0].InnerBaseline) ? lineBoxes[0].InnerBaseline + toContent.Top : nuPosNULL;
	out.NodeWidth = contentWidth + border.Left + border.Right + margin.Left + margin.Right + padding.Left + padding.Right;
	out.NodeHeight = contentHeight + border.Top + border.Bottom + margin.Top + margin.Bottom + padding.Top + padding.Bottom;
	out.Binds = ComputeBinds();
	out.Break = Stack.Get( nuCatBreak ).GetBreakType();

	Stack.StackPop();
}

void nuLayout2::RunText( const nuDomText& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomText* rnode )
{
	NUTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 1\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();
	rnode->SetStyle( Stack );

	NUTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 2\n", node.GetInternalID() );

	rnode->FontID = Stack.Get( nuCatFontFamily ).GetFont();

	nuStyleAttrib fontSizeAttrib = Stack.Get( nuCatFontSize );
	nuPos fontHeight = ComputeDimension( in.ParentHeight, fontSizeAttrib.GetSize() );

	float fontSizePxUnrounded = nuPosToReal( fontHeight );
	
	// round font size to integer units
	rnode->FontSizePx = (uint8) nuRound( fontSizePxUnrounded );

	out.NodeWidth = 0;
	out.NodeHeight = 0;
	out.NodeBaseline = 0;

	// Nothing prevents somebody from setting a font size to zero
	if ( rnode->FontSizePx < 1 )
		return;

	bool subPixel = nuGlobal()->EnableSubpixelText && rnode->FontSizePx <= nuGlobal()->MaxSubpixelGlyphSize;
	if ( subPixel )
		rnode->Flags |= nuRenderDomText::FlagSubPixelGlyphs;

	TempText.Node = &node;
	TempText.RNode = rnode;
	TempText.Words.clear_noalloc();
	TempText.GlyphCount = 0;
	//TempText.FontWidthScale = 1.08f;	// Verdana looks a lot better with an extra 8%
	//TempText.FontWidthScale = 1.05f;
	TempText.FontWidthScale = 1.0f;
	GenerateTextWords( TempText );
	if ( !TempText.GlyphsNeeded )
		GenerateTextOutput( in, out, TempText );
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
void nuLayout2::GenerateTextOutput( const LayoutInput& in, LayoutOutput& out, TextRunState& ts )
{
	const char* txt = ts.Node->GetText();
	nuGlyphCache* glyphCache = nuGlobal()->GlyphCache;
	nuGlyphCacheKey key = MakeGlyphCacheKey( ts.RNode );
	const nuFont* font = Fonts.GetByFontID( ts.RNode->FontID );

	nuPos fontHeightRounded = nuRealToPos( ts.RNode->FontSizePx );
	nuPos fontAscender = nuRealx256ToPos( font->Ascender_x256 * ts.RNode->FontSizePx );
	//nuTextAlignVertical valign = Stack.Get( nuCatText_Align_Vertical ).GetTextAlignVertical();
	
	// if we add a "line-height" style then we'll want to multiply that by this
	nuPos lineHeight = nuRealx256ToPos( ts.RNode->FontSizePx * font->LineHeight_x256 ); 
	if ( nuGlobal()->RoundLineHeights )
		lineHeight = nuPosRoundUp( lineHeight );

	int fontHeightPx = ts.RNode->FontSizePx;
	ts.RNode->Text.reserve( ts.GlyphCount );
	bool parentHasWidth = IsDefined(in.ParentWidth);
	bool enableKerning = nuGlobal()->EnableKerning;

	//nuPos baseline = 0;
	//if ( valign == nuTextAlignVerticalTop || s.PosBaselineY == nuPosNULL )		baseline = s.PosY + fontAscender;
	//else if ( valign == nuTextAlignVerticalBaseline )							baseline = s.PosBaselineY;
	//else																		NUTODO;
	
	// First text in the line defines the baseline
	//if ( s.PosBaselineY == nuPosNULL )
	//	s.PosBaselineY = baseline;
	if ( strcmp(txt, "world") == 0 )
		int abc = 123;

	nuPos posX = 0;
	nuPos posMaxX = 0;
	nuPos posMaxY = 0;
	//nuPos baseline = IsDefined(in.ParentBaseline) ? in.ParentBaseline : fontAscender;
	nuPos baseline = fontAscender;
	out.NodeBaseline = baseline;

	for ( intp iword = 0; iword < ts.Words.size(); iword++ )
	{
		const Word& word = ts.Words[iword];
		bool isSpace = word.Length() == 1 && txt[word.Start] == 32;
		bool isNewline = word.Length() == 1 && txt[word.Start] == '\n';
		//bool over = parentHasWidth ? s.PosX + word.Width > s.ParentContentBox.Right : false;
		bool over = parentHasWidth ? posX + word.Width > in.ParentWidth : false;
		if ( over )
		{
			bool futile = posX == 0;
			if ( !futile )
			{
				//NextLine( s );
				baseline += lineHeight;
				posX = 0;
				// If the line break was performed for a space, then treat that space as "done"
				if ( isSpace )
					continue;
			}
		}

		if ( isSpace )
		{
			posX += nuRealx256ToPos( font->LinearHoriAdvance_Space_x256 ) * fontHeightPx;
		}
		else if ( isNewline )
		{
			//NextLine( s );
			baseline += lineHeight;
			posX = 0;
		}
		else
		{
			const nuGlyph* prevGlyph = nullptr;
			for ( intp i = word.Start; i < word.End; i++ )
			{
				key.Char = txt[i];
				const nuGlyph* glyph = glyphCache->GetGlyph( key );
				__analysis_assume( glyph != nullptr );
				if ( glyph->IsNull() )
					continue;
				if ( enableKerning && prevGlyph )
				{
					// Multithreading hazard here. I'm not sure whether FT_Get_Kerning is thread safe.
					// Also, I have stepped inside there and I see it does a binary search. We would probably
					// be better off caching the kerning for frequent pairs of glyphs in a hash table.
					FT_Vector kern;
					FT_Get_Kerning( font->FTFace, prevGlyph->FTGlyphIndex, glyph->FTGlyphIndex, FT_KERNING_UNSCALED, &kern );
					nuPos kerning = ((kern.x * fontHeightPx) << nuPosShift) / font->FTFace->units_per_EM;
					posX += kerning;
				}
				ts.RNode->Text.Count++;
				nuRenderCharEl& rtxt = ts.RNode->Text.back();
				rtxt.Char = key.Char;
				rtxt.X = posX + nuRealx256ToPos( glyph->MetricLeftx256 );
				rtxt.Y = baseline - nuRealToPos( glyph->MetricTop );			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
				posX += HoriAdvance( glyph, ts );
				posMaxX = nuMax( posMaxX, posX );
				prevGlyph = glyph;
			}
		}
		posMaxY = nuMax( posMaxY, baseline - fontAscender + lineHeight );
	}

	out.NodeWidth = posMaxX;
	out.NodeHeight = posMaxY;
}

void nuLayout2::GenerateTextWords( TextRunState& ts )
{
	const char* txt = ts.Node->GetText();
	nuGlyphCache* glyphCache = nuGlobal()->GlyphCache;
	nuGlyphCacheKey key = MakeGlyphCacheKey( ts.RNode );

	ts.GlyphsNeeded = false;
	bool onSpace = false;
	Word word;
	word.Start = 0;
	word.Width = 0;
	for ( intp i = 0; true; i++ )
	{
		bool isSpace = IsSpace(txt[i]) || IsLinebreak(txt[i]);
		if ( isSpace || onSpace || txt[i] == 0 ) 
		{
			word.End = (int32) i;
			if ( word.End != word.Start )
				ts.Words += word;
			word.Start = (int32) i;
			word.Width = 0;
			onSpace = isSpace;
		}
		if ( txt[i] == 0 )
			break;
		key.Char = txt[i];
		const nuGlyph* glyph = glyphCache->GetGlyph( key );
		if ( !glyph )
		{
			ts.GlyphsNeeded = true;
			GlyphsNeeded.insert( key );
			continue;
		}
		if ( glyph->IsNull() )
		{
			// TODO: Handle missing glyph by drawing a rectangle or something
			continue;
		}
		ts.GlyphCount++;
		word.Width += HoriAdvance( glyph, ts );
	}
}

nuPoint nuLayout2::PositionChildFromBindings( const LayoutInput& cin, const LayoutOutput& cout, nuRenderDomEl* rchild )
{
	nuPoint child, parent;
	child.X = HBindOffset( cout.Binds.HChild, cout.NodeWidth );
	child.Y = VBindOffset( cout.Binds.VChild, cout.NodeBaseline, cout.NodeHeight );
	parent.X = HBindOffset( cout.Binds.HParent, cin.ParentWidth );
	parent.Y = VBindOffset( cout.Binds.VParent, cin.OuterBaseline, cin.ParentHeight );
	nuPoint offset( parent.X - child.X, parent.Y - child.Y );
	rchild->Pos.Offset( offset );
	return offset;
}

nuPos nuLayout2::ComputeDimension( nuPos container, nuStyleCategories cat )
{
	return ComputeDimension( container, Stack.Get( cat ).GetSize() );
}

nuPos nuLayout2::ComputeDimension( nuPos container, nuSize size )
{
	switch ( size.Type )
	{
	case nuSize::NONE: return nuPosNULL;
	case nuSize::PX: return nuRealToPos( size.Val );
	case nuSize::PT: return nuRealToPos( size.Val * PtToPixel );
	case nuSize::EP: return nuRealToPos( size.Val * EpToPixel );
	case nuSize::PERCENT:
		if ( container == nuPosNULL )
			return nuPosNULL;
		else
			return nuPos( nuRound((float) container * (size.Val * 0.01f)) ); // this might be sloppy floating point. Small rational percentages like 25% (1/4) ought to be preserved precisely.
	default: NUPANIC("Unrecognized size type"); return 0;
	}
}

nuBox nuLayout2::ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleCategories cat )
{
	return ComputeBox( containerWidth, containerHeight, Stack.GetBox( cat ) );
}

nuBox nuLayout2::ComputeBox( nuPos containerWidth, nuPos containerHeight, nuStyleBox box )
{
	nuBox b;
	b.Left = ComputeDimension( containerWidth, box.Left );
	b.Right = ComputeDimension( containerWidth, box.Right );
	b.Top = ComputeDimension( containerHeight, box.Top );
	b.Bottom = ComputeDimension( containerHeight, box.Bottom );
	return b;
}

nuLayout2::BindingSet nuLayout2::ComputeBinds()
{
	nuHorizontalBindings left = Stack.Get( nuCatLeft ).GetHorizontalBinding();
	nuHorizontalBindings hcenter = Stack.Get( nuCatHCenter ).GetHorizontalBinding();
	nuHorizontalBindings right = Stack.Get( nuCatRight ).GetHorizontalBinding();

	nuVerticalBindings top = Stack.Get( nuCatTop ).GetVerticalBinding();
	nuVerticalBindings vcenter = Stack.Get( nuCatVCenter ).GetVerticalBinding();
	nuVerticalBindings bottom = Stack.Get( nuCatBottom ).GetVerticalBinding();
	nuVerticalBindings baseline = Stack.Get( nuCatBaseline ).GetVerticalBinding();

	BindingSet binds = {nuHorizontalBindingNULL, nuHorizontalBindingNULL, nuVerticalBindingNULL, nuVerticalBindingNULL};

	if ( left != nuHorizontalBindingNULL )		{ binds.HChild = nuHorizontalBindingLeft; binds.HParent = left; }
	if ( hcenter != nuHorizontalBindingNULL )	{ binds.HChild = nuHorizontalBindingCenter; binds.HParent = hcenter; }
	if ( right != nuHorizontalBindingNULL )		{ binds.HChild = nuHorizontalBindingRight; binds.HParent = right; }

	if ( top != nuVerticalBindingNULL )			{ binds.VChild = nuVerticalBindingTop; binds.VParent = top; }
	if ( vcenter != nuVerticalBindingNULL )		{ binds.VChild = nuVerticalBindingCenter; binds.VParent = vcenter; }
	if ( bottom != nuVerticalBindingNULL )		{ binds.VChild = nuVerticalBindingBottom; binds.VParent = bottom; }
	if ( baseline != nuVerticalBindingNULL )	{ binds.VChild = nuVerticalBindingBaseline; binds.VParent = baseline; }

	return binds;
}

nuPos nuLayout2::HoriAdvance( const nuGlyph* glyph, const TextRunState& ts )
{
	if ( SnapSubpixelHorzText )
		return nuIntToPos( glyph->MetricHoriAdvance );
	else
		return nuRealToPos( glyph->MetricLinearHoriAdvance * ts.FontWidthScale );
}

nuPos nuLayout2::HBindOffset( nuHorizontalBindings bind, nuPos width )
{
	switch ( bind )
	{
	case nuHorizontalBindingNULL:
	case nuHorizontalBindingLeft:	return 0;
	case nuHorizontalBindingCenter:	return width / 2;
	case nuHorizontalBindingRight:	return width;
	default:
		NUASSERTDEBUG(false);
		return 0;
	}
}

nuPos nuLayout2::VBindOffset( nuVerticalBindings bind, nuPos baseline, nuPos height )
{
	switch ( bind )
	{
	case nuVerticalBindingNULL:
	case nuVerticalBindingTop:		return 0;
	case nuVerticalBindingCenter:	return height / 2;
	case nuVerticalBindingBottom:	return height;
	case nuVerticalBindingBaseline:
		if ( IsDefined(baseline) )
			return baseline;
		else
		{
			NUTRACE_LAYOUT_WARNING( "Undefined baseline used in alignment\n" );
			return height;
		}
	default:
		NUASSERTDEBUG(false);
		return 0;
	}
}

bool nuLayout2::IsSpace( int ch )
{
	return ch == 32;
}

bool nuLayout2::IsLinebreak( int ch )
{
	return ch == '\r' || ch == '\n';
}

nuGlyphCacheKey	nuLayout2::MakeGlyphCacheKey( nuRenderDomText* rnode )
{
	uint8 glyphFlags = rnode->IsSubPixel() ? nuGlyphFlag_SubPixel_RGB : 0;
	return nuGlyphCacheKey( rnode->FontID, 0, rnode->FontSizePx, glyphFlags );
}

void nuLayout2::FlowNewline( FlowState& flow )
{
	flow.PosMinor = 0;
	flow.PosMajor = flow.MajorMax;
	flow.NumLines++;
}

bool nuLayout2::FlowBreakBefore( const LayoutOutput& cout, FlowState& flow )
{
	nuBreakType breakType = nuBreakType(cout.Break & 3);	// need to mask off because of enum sign extension
	if ( breakType == nuBreakBefore )
	{
		FlowNewline( flow );
		return true;
	}
	return false;
}

nuPoint nuLayout2::FlowRun( const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, nuRenderDomEl* rendEl )
{
	nuBreakType breakType = nuBreakType(cout.Break & 3);	// need to mask off because of enum sign extension

	if ( IsDefined(cin.ParentWidth) )
	{
		bool over = flow.PosMinor + cout.NodeWidth > cin.ParentWidth;
		bool futile = flow.PosMinor == 0;
		if ( over && !futile )
			FlowNewline( flow );
	}
	nuPoint offset( flow.PosMinor, flow.PosMajor );
	rendEl->Pos.Offset( offset.X, offset.Y );
	flow.PosMinor += cout.NodeWidth;
	flow.MajorMax = nuMax( flow.MajorMax, flow.PosMajor + cout.NodeHeight );

	if ( breakType == nuBreakAfter )
		FlowNewline( flow );

	return offset;
}
