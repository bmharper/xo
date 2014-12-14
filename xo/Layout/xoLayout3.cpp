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
void xoLayout3::Layout( const xoDoc& doc, xoRenderDomNode& root, xoPool* pool )
{
	Doc = &doc;
	Pool = pool;
	Boxer.Pool = pool;
	Stack.Initialize( Doc, Pool );
	ChildOutStack.Init( 1024 * 1024 );
	LineBoxStack.Init( 1024 * 1024 );
	Fonts = xoGlobal()->FontStore->GetImmutableTable();
	SnapBoxes = xoGlobal()->SnapBoxes;
	SnapSubpixelHorzText = xoGlobal()->SnapSubpixelHorzText;

	while ( true )
	{
		LayoutInternal( root );

		if ( GlyphsNeeded.size() == 0 )
		{
			XOTRACE_LAYOUT_VERBOSE( "Layout done\n" );
			break;
		}
		else
		{
			XOTRACE_LAYOUT_VERBOSE( "Layout done (but need another pass for missing glyphs)\n" );
			RenderGlyphsNeeded();
		}
	}
}

void xoLayout3::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		xoGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

void xoLayout3::LayoutInternal( xoRenderDomNode& root )
{
	PtToPixel = 1.0;	// TODO
	EpToPixel = xoGlobal()->EpToPixel;

	XOTRACE_LAYOUT_VERBOSE( "Layout 1\n" );

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	XOTRACE_LAYOUT_VERBOSE( "Layout 2\n" );

	LayoutInput3 in;
	in.ParentWidth = xoIntToPos( Doc->UI.GetViewportWidth() );
	in.ParentHeight = xoIntToPos( Doc->UI.GetViewportHeight() );

	//Boxer.BeginDocument( Doc->UI.GetViewportWidth(), Doc->UI.GetViewportHeight(), &root );
	Boxer.BeginDocument( &root );
	RunNode3( &Doc->Root, in );
	Boxer.EndDocument();

	//LayoutInput in;
	//in.ParentWidth = xoIntToPos( Doc->UI.GetViewportWidth() );
	//in.ParentHeight = xoIntToPos( Doc->UI.GetViewportHeight() );
	//in.OuterBaseline = xoPosNULL;
	//
	//LayoutOutput out;
	//
	//XOTRACE_LAYOUT_VERBOSE( "Layout 3 DocBox = %d,%d\n", in.ParentWidth, in.ParentHeight );
	//
	//RunNode( Doc->Root, in, out, &root );
}

void xoLayout3::RunNode3( const xoDomNode* node, const LayoutInput3& in )
{
	xoBoxLayout3::NodeInput boxIn;

	xoStyleResolver::ResolveAndPush( Stack, node );
	//rnode->SetStyle( Stack );

	xoPos contentWidth = ComputeDimension( in.ParentWidth, xoCatWidth );
	xoPos contentHeight = ComputeDimension( in.ParentHeight, xoCatHeight );

	boxIn.InternalID = node->GetInternalID();
	boxIn.Tag = node->GetTag();
	boxIn.ContentWidth = contentWidth;
	boxIn.ContentHeight = contentHeight;
	boxIn.NewFlowContext = Stack.Get( xoCatFlowContext ).GetFlowContext() == xoFlowContextNew;

	LayoutInput3 childIn;
	childIn.ParentWidth = contentWidth;
	childIn.ParentHeight = contentHeight;

	Boxer.BeginNode( boxIn );

	for ( intp i = 0; i < node->ChildCount(); i++ )
	{
		const xoDomEl* c = node->ChildByIndex(i);
		if ( c->IsNode() )
		{
			RunNode3( static_cast<const xoDomNode*>(c), childIn );
		}
		else
		{
			RunText3( static_cast<const xoDomText*>(c), childIn );
		}
	}
	Boxer.EndNode();
}

void xoLayout3::RunNode( const xoDomNode& node, const LayoutInput& in, LayoutOutput& out, xoRenderDomNode* rnode )
{
	XOTRACE_LAYOUT_VERBOSE( "Layout (%d) Run 1\n", node.GetInternalID() );
	xoStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );

	XOTRACE_LAYOUT_VERBOSE( "Layout (%d) Run 2\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();

	xoBoxSizeType boxSizing = Stack.Get( xoCatBoxSizing ).GetBoxSizing();
	xoPos borderRadius = ComputeDimension( 0, xoCatBorderRadius );
	xoPos contentWidth = ComputeDimension( in.ParentWidth, xoCatWidth );
	xoPos contentHeight = ComputeDimension( in.ParentHeight, xoCatHeight );
	xoBox margin = ComputeBox( in.ParentWidth, in.ParentHeight, xoCatMargin_Left );		// it may be wise to disallow percentage sizing here
	xoBox padding = ComputeBox( in.ParentWidth, in.ParentHeight, xoCatPadding_Left );	// same here
	xoBox border = ComputeBox( in.ParentWidth, in.ParentHeight, xoCatBorder_Left );		// and here
	
	// It might make for less arithmetic if we work with marginBoxWidth and marginBoxHeight instead of contentBoxWidth and contentBoxHeight. We'll see.

	// This box holds the offsets from the 4 sides of our origin, to our content box. (Our origin is our parent's content box, but since it's relative here, it starts at 0,0)
	xoBox toContent;
	toContent.Left = margin.Left + border.Left + padding.Left;
	toContent.Right = margin.Right + border.Right + padding.Right;
	toContent.Top = margin.Top + border.Top + padding.Top;
	toContent.Bottom = margin.Bottom + border.Bottom + padding.Bottom;

	if ( boxSizing == xoBoxSizeContent ) {}
	else if ( boxSizing == xoBoxSizeBorder )
	{
		if ( IsDefined(contentWidth) )	contentWidth -= border.Left + border.Right + padding.Left + padding.Right;
		if ( IsDefined(contentHeight) )	contentHeight -= border.Top + border.Bottom + padding.Top + padding.Bottom;
	}
	else if ( boxSizing == xoBoxSizeMargin )
	{
		if ( IsDefined(contentWidth) )	contentWidth -= margin.Left + margin.Right + border.Left + border.Right + padding.Left + padding.Right;
		if ( IsDefined(contentHeight) )	contentHeight -= margin.Top + margin.Bottom + border.Top + border.Bottom + padding.Top + padding.Bottom;
	}

	if ( SnapBoxes )
	{
		if ( IsDefined(contentWidth) )	contentWidth = xoPosRoundUp( contentWidth );
		if ( IsDefined(contentHeight) )	contentHeight = xoPosRoundUp( contentHeight );
	}

	xoPos autoWidth = 0;
	xoPos autoHeight = 0;
	xoPos outerBaseline = IsDefined(in.OuterBaseline) ? in.OuterBaseline - toContent.Top : xoPosNULL;

	// If we don't know our width and height yet then we need to delay bindings until our first pass is done
	// The buffer size of 16 here is thumbsuck. One can't make it too big, because this is a recursive function.
	// -- I first tried to have an optimized case for binding during first pass, but I have given up on that.
	// -- One could revisit it once the design is nailed down.
	xoLifoVector<LayoutOutput> outs( ChildOutStack );
	outs.AddN( node.ChildCount() );

	xoLifoVector<LineBox> lineBoxes( LineBoxStack );
	lineBoxes.Push( LineBox::Make( xoPosNULL, -1, INT32MAX ) );

	FlowState flow;
	flow.PosMajor = 0;
	flow.PosMinor = 0;
	flow.MajorMax = 0;
	flow.NumLines = 0;

	for ( intp i = 0; i < node.ChildCount(); i++ )
	{
		const xoDomEl* c = node.ChildByIndex( i );
		LayoutInput cin;
		LayoutOutput cout;
		cin.OuterBaseline = IsDefined(outerBaseline) ? outerBaseline : lineBoxes.Back().InnerBaseline;
		cin.ParentWidth = contentWidth;
		cin.ParentHeight = contentHeight;
		xoPoint offset(0,0);
		int nlines = flow.NumLines;
		bool breakBefore = false;
		if ( c->GetTag() == xoTagText )
		{
			xoRenderDomText* rchildTxt = new (Pool->AllocT<xoRenderDomText>(false)) xoRenderDomText( c->GetInternalID(), Pool );
			rnode->Children += rchildTxt;
			RunText( *static_cast<const xoDomText*>(c), cin, cout, rchildTxt );
			breakBefore = FlowBreakBefore( cout, flow );
			offset += FlowRun( cin, cout, flow, rchildTxt );
			// Text elements cannot choose their layout. They are forced to start in the top-left of their parent, and perform text layout inside that space.
		}
		else
		{
			xoRenderDomNode* rchildNode = new (Pool->AllocT<xoRenderDomNode>(false)) xoRenderDomNode( c->GetInternalID(), c->GetTag(), Pool );
			rnode->Children += rchildNode;
			RunNode( *static_cast<const xoDomNode*>(c), cin, cout, rchildNode );
			breakBefore = FlowBreakBefore( cout, flow );
			offset += FlowRun( cin, cout, flow, rchildNode );
		}
		outs[i] = cout;
		if ( flow.NumLines != nlines && breakBefore )
		{
			// Create a new linebox BEFORE adding this child's state
			lineBoxes.Back().LastChild = int(i - 1);
			lineBoxes += LineBox::Make( xoPosNULL, -1, INT32MAX );
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
			lineBoxes += LineBox::Make( xoPosNULL, -1, INT32MAX );
		}
		autoWidth = xoMax( autoWidth, flow.PosMinor );
		autoHeight = xoMax( autoHeight, flow.MajorMax );
	}

	if ( IsNull(contentWidth) ) contentWidth = SnapBoxes ? xoPosRoundUp(autoWidth) : autoWidth;
	if ( IsNull(contentHeight) ) contentHeight = SnapBoxes ?  xoPosRoundUp(autoHeight) : autoHeight;

	// Apply bindings
	{
		int iLineBox = 0;
		LayoutInput cin;
		cin.OuterBaseline = IsDefined(outerBaseline) ? outerBaseline : lineBoxes[iLineBox].InnerBaseline;
		cin.ParentWidth = contentWidth;
		cin.ParentHeight = contentHeight;
		for ( intp i = 0; i < node.ChildCount(); i++ )
		{
			const xoDomEl* c = node.ChildByIndex( i );
			xoPoint offset(0,0);
			if ( c->GetTag() != xoTagText )
				offset = PositionChildFromBindings( cin, outs[i], rnode->Children[i] );
			if ( i == lineBoxes[iLineBox].InnerBaselineDefinedBy )
				lineBoxes[iLineBox].InnerBaseline += offset.Y;
			if ( lineBoxes[iLineBox].LastChild == i )
				iLineBox++;
		}
	}

	rnode->Pos = xoBox( 0, 0, contentWidth, contentHeight ).OffsetBy( toContent.Left, toContent.Top );
	rnode->Style.BackgroundColor = Stack.Get( xoCatBackground ).GetColor();
	rnode->Style.BorderRadius = xoPosToReal( borderRadius );
	rnode->Style.BorderSize = border;
	rnode->Style.Padding = padding;
	rnode->Style.BorderColor = Stack.Get( xoCatBorderColor_Left ).GetColor();
	rnode->Style.HasHoverStyle = Stack.HasHoverStyle();
	rnode->Style.HasFocusStyle = Stack.HasFocusStyle();

	out.NodeBaseline = IsDefined(lineBoxes[0].InnerBaseline) ? lineBoxes[0].InnerBaseline + toContent.Top : xoPosNULL;
	out.NodeWidth = contentWidth + border.Left + border.Right + margin.Left + margin.Right + padding.Left + padding.Right;
	out.NodeHeight = contentHeight + border.Top + border.Bottom + margin.Top + margin.Bottom + padding.Top + padding.Bottom;
	out.Binds = ComputeBinds();
	out.Break = Stack.Get( xoCatBreak ).GetBreakType();
	//out.Position = Stack.Get( xoCatPosition ).GetPositionType();

	Stack.StackPop();
}

void xoLayout3::RunText3( const xoDomText* node, const LayoutInput3& in )
{
	//XOTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 1\n", node.GetInternalID() );
	//rnode->InternalID = node.GetInternalID();
	//rnode->SetStyle( Stack );

	//XOTRACE_LAYOUT_VERBOSE( "Layout text (%d) Run 2\n", node.GetInternalID() );

	//rnode->FontID = Stack.Get( xoCatFontFamily ).GetFont();
	xoFontID fontID = Stack.Get( xoCatFontFamily ).GetFont();

	xoStyleAttrib fontSizeAttrib = Stack.Get( xoCatFontSize );
	xoPos fontHeight = ComputeDimension( in.ParentHeight, fontSizeAttrib.GetSize() );

	float fontSizePxUnrounded = xoPosToReal( fontHeight );
	
	// round font size to integer units
	//rnode->FontSizePx = (uint8) xoRound( fontSizePxUnrounded );
	uint8 fontSizePx = (uint8) xoRound( fontSizePxUnrounded );

	//out.NodeWidth = 0;
	//out.NodeHeight = 0;
	//out.NodeBaseline = 0;

	// Nothing prevents somebody from setting a font size to zero
	//if ( rnode->FontSizePx < 1 )
	if ( fontSizePx < 1 )
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
	TempText.Color = Stack.Get( xoCatColor ).GetColor();
	GenerateTextWords( TempText );

	//Boxer.EndNode();
}

void xoLayout3::RunText( const xoDomText& node, const LayoutInput& in, LayoutOutput& out, xoRenderDomText* rnode )
{
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
void xoLayout3::GenerateTextOutput( const LayoutInput& in, LayoutOutput& out, TextRunState& ts )
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey( ts.RNode );
	const xoFont* font = Fonts.GetByFontID( ts.RNode->FontID );

	xoPos fontHeightRounded = xoRealToPos( ts.RNode->FontSizePx );
	xoPos fontAscender = xoRealx256ToPos( font->Ascender_x256 * ts.RNode->FontSizePx );
	
	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos( ts.RNode->FontSizePx * font->LineHeight_x256 ); 
	if ( xoGlobal()->RoundLineHeights )
		lineHeight = xoPosRoundUp( lineHeight );

	int fontHeightPx = ts.RNode->FontSizePx;
	ts.RNode->Text.reserve( ts.GlyphCount );
	bool parentHasWidth = IsDefined(in.ParentWidth);
	bool enableKerning = xoGlobal()->EnableKerning;

	xoPos posX = 0;
	xoPos posMaxX = 0;
	xoPos posMaxY = 0;
	xoPos baseline = fontAscender;
	out.NodeBaseline = baseline;

	for ( intp iword = 0; iword < ts.Words.size(); iword++ )
	{
		const Word& word = ts.Words[iword];
		bool isSpace = word.Length() == 1 && txt[word.Start] == 32;
		bool isNewline = word.Length() == 1 && txt[word.Start] == '\n';
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
			posX += xoRealx256ToPos( font->LinearHoriAdvance_Space_x256 ) * fontHeightPx;
		}
		else if ( isNewline )
		{
			//NextLine( s );
			baseline += lineHeight;
			posX = 0;
		}
		else
		{
			const xoGlyph* prevGlyph = nullptr;
			for ( intp i = word.Start; i < word.End; i++ )
			{
				key.Char = txt[i];
				const xoGlyph* glyph = glyphCache->GetGlyph( key );
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
					xoPos kerning = ((kern.x * fontHeightPx) << xoPosShift) / font->FTFace->units_per_EM;
					posX += kerning;
				}
				ts.RNode->Text.Count++;
				xoRenderCharEl& rtxt = ts.RNode->Text.back();
				rtxt.Char = key.Char;
				rtxt.X = posX + xoRealx256ToPos( glyph->MetricLeftx256 );
				rtxt.Y = baseline - xoRealToPos( glyph->MetricTop );			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
				posX += HoriAdvance( glyph, ts );
				posMaxX = xoMax( posMaxX, posX );
				prevGlyph = glyph;
			}
		}
		posMaxY = xoMax( posMaxY, baseline - fontAscender + lineHeight );
	}

	out.NodeWidth = posMaxX;
	out.NodeHeight = posMaxY;
}

void xoLayout3::GenerateTextWords( TextRunState& ts )
{
	const char* txt = ts.Node->GetText();
	xoGlyphCache* glyphCache = xoGlobal()->GlyphCache;
	xoGlyphCacheKey key = MakeGlyphCacheKey( ts );
	const xoFont* font = Fonts.GetByFontID( ts.FontID );

	xoPos fontHeightRounded = xoRealToPos( (float) ts.FontSizePx );
	xoPos fontAscender = xoRealx256ToPos( font->Ascender_x256 * ts.FontSizePx );
	
	// if we add a "line-height" style then we'll want to multiply that by this
	xoPos lineHeight = xoRealx256ToPos( ts.FontSizePx * font->LineHeight_x256 ); 
	if ( xoGlobal()->RoundLineHeights )
		lineHeight = xoPosRoundUp( lineHeight );

	ts.GlyphsNeeded = false;
	bool onSpace = false;
	intp wordStart = 0;
	xoPos posX = 0;
	xoPos baseline = fontAscender;
	const xoGlyph* prevGlyph = nullptr;
	for ( intp i = 0; true; i++ )
	{
		bool isSpace = IsSpace(txt[i]) || IsLinebreak(txt[i]);
		if ( isSpace || onSpace || txt[i] == 0 ) 
		{
			if ( i != wordStart && !ts.GlyphsNeeded )
			{
				xoBoxLayout3::WordInput wordin;
				wordin.Width = posX;
				wordin.Height = lineHeight; // unsure
				xoRenderDomText* rtxt = Boxer.AddWord( wordin );
				AddWordCharacters( ts, rtxt );
				ts.Chars.clear_noalloc();
			}
			wordStart = (int32) i;
			posX = 0;
			onSpace = isSpace;
		}
		if ( txt[i] == 0 )
			break;
		key.Char = txt[i];
		const xoGlyph* glyph = glyphCache->GetGlyph( key );
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
		if ( xoGlobal()->EnableKerning && prevGlyph )
		{
			// Multithreading hazard here. I'm not sure whether FT_Get_Kerning is thread safe.
			// Also, I have stepped inside there and I see it does a binary search. We would probably
			// be better off caching the kerning for frequent pairs of glyphs in a hash table.
			FT_Vector kern;
			FT_Get_Kerning( font->FTFace, prevGlyph->FTGlyphIndex, glyph->FTGlyphIndex, FT_KERNING_UNSCALED, &kern );
			xoPos kerning = ((kern.x * ts.FontSizePx) << xoPosShift) / font->FTFace->units_per_EM;
			posX += kerning;
		}

		xoRenderCharEl& rtxt = ts.Chars.add();
		rtxt.Char = key.Char;
		rtxt.X = posX + xoRealx256ToPos( glyph->MetricLeftx256 );
		rtxt.Y = baseline - xoRealToPos( glyph->MetricTop );			// rtxt.Y is the top of the glyph bitmap. glyph->MetricTop is the distance from the baseline to the top of the glyph
		// For determining the word width, one might want to not use the horizontal advance for the very last glyph, but instead
		// use the glyph's exact width. The difference would be tiny, and it may even be annoying, because you would end up with
		// no padding on the right side of a word.
		posX += HoriAdvance( glyph, ts );
	}
}

void xoLayout3::AddWordCharacters( const TextRunState& ts, xoRenderDomText* rnode )
{
	rnode->FontID = ts.FontID;
	rnode->Color = ts.Color;
	rnode->FontSizePx = ts.FontSizePx;
	if ( ts.IsSubPixel )
		rnode->Flags |= xoRenderDomText::FlagSubPixelGlyphs;

	rnode->Text.resize( ts.Chars.size() );
	for ( intp i = 0; i < ts.Chars.size(); i++ )
		rnode->Text[i] = ts.Chars[i];
}

xoPoint xoLayout3::PositionChildFromBindings( const LayoutInput& cin, const LayoutOutput& cout, xoRenderDomEl* rchild )
{
	xoPoint child, parent;
	child.X = HBindOffset( cout.Binds.HChild, cout.NodeWidth );
	child.Y = VBindOffset( cout.Binds.VChild, cout.NodeBaseline, cout.NodeHeight );
	parent.X = HBindOffset( cout.Binds.HParent, cin.ParentWidth );
	parent.Y = VBindOffset( cout.Binds.VParent, cin.OuterBaseline, cin.ParentHeight );
	xoPoint offset( parent.X - child.X, parent.Y - child.Y );
	rchild->Pos.Offset( offset );
	return offset;
}

xoPos xoLayout3::ComputeDimension( xoPos container, xoStyleCategories cat )
{
	return ComputeDimension( container, Stack.Get( cat ).GetSize() );
}

xoPos xoLayout3::ComputeDimension( xoPos container, xoSize size )
{
	switch ( size.Type )
	{
	case xoSize::NONE: return xoPosNULL;
	case xoSize::PX: return xoRealToPos( size.Val );
	case xoSize::PT: return xoRealToPos( size.Val * PtToPixel );
	case xoSize::EP: return xoRealToPos( size.Val * EpToPixel );
	case xoSize::PERCENT:
		if ( container == xoPosNULL )
			return xoPosNULL;
		else
			return xoPos( xoRound((float) container * (size.Val * 0.01f)) ); // this might be sloppy floating point. Small rational percentages like 25% (1/4) ought to be preserved precisely.
	default: XOPANIC("Unrecognized size type"); return 0;
	}
}

xoBox xoLayout3::ComputeBox( xoPos containerWidth, xoPos containerHeight, xoStyleCategories cat )
{
	return ComputeBox( containerWidth, containerHeight, Stack.GetBox( cat ) );
}

xoBox xoLayout3::ComputeBox( xoPos containerWidth, xoPos containerHeight, xoStyleBox box )
{
	xoBox b;
	b.Left = ComputeDimension( containerWidth, box.Left );
	b.Right = ComputeDimension( containerWidth, box.Right );
	b.Top = ComputeDimension( containerHeight, box.Top );
	b.Bottom = ComputeDimension( containerHeight, box.Bottom );
	return b;
}

xoLayout3::BindingSet xoLayout3::ComputeBinds()
{
	xoHorizontalBindings left = Stack.Get( xoCatLeft ).GetHorizontalBinding();
	xoHorizontalBindings hcenter = Stack.Get( xoCatHCenter ).GetHorizontalBinding();
	xoHorizontalBindings right = Stack.Get( xoCatRight ).GetHorizontalBinding();

	xoVerticalBindings top = Stack.Get( xoCatTop ).GetVerticalBinding();
	xoVerticalBindings vcenter = Stack.Get( xoCatVCenter ).GetVerticalBinding();
	xoVerticalBindings bottom = Stack.Get( xoCatBottom ).GetVerticalBinding();
	xoVerticalBindings baseline = Stack.Get( xoCatBaseline ).GetVerticalBinding();

	BindingSet binds = {xoHorizontalBindingNULL, xoHorizontalBindingNULL, xoVerticalBindingNULL, xoVerticalBindingNULL};

	if ( left != xoHorizontalBindingNULL )		{ binds.HChild = xoHorizontalBindingLeft; binds.HParent = left; }
	if ( hcenter != xoHorizontalBindingNULL )	{ binds.HChild = xoHorizontalBindingCenter; binds.HParent = hcenter; }
	if ( right != xoHorizontalBindingNULL )		{ binds.HChild = xoHorizontalBindingRight; binds.HParent = right; }

	if ( top != xoVerticalBindingNULL )			{ binds.VChild = xoVerticalBindingTop; binds.VParent = top; }
	if ( vcenter != xoVerticalBindingNULL )		{ binds.VChild = xoVerticalBindingCenter; binds.VParent = vcenter; }
	if ( bottom != xoVerticalBindingNULL )		{ binds.VChild = xoVerticalBindingBottom; binds.VParent = bottom; }
	if ( baseline != xoVerticalBindingNULL )	{ binds.VChild = xoVerticalBindingBaseline; binds.VParent = baseline; }

	return binds;
}

xoPos xoLayout3::HoriAdvance( const xoGlyph* glyph, const TextRunState& ts )
{
	if ( SnapSubpixelHorzText )
		return xoIntToPos( glyph->MetricHoriAdvance );
	else
		return xoRealToPos( glyph->MetricLinearHoriAdvance * ts.FontWidthScale );
}

xoPos xoLayout3::HBindOffset( xoHorizontalBindings bind, xoPos width )
{
	switch ( bind )
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

xoPos xoLayout3::VBindOffset( xoVerticalBindings bind, xoPos baseline, xoPos height )
{
	switch ( bind )
	{
	case xoVerticalBindingNULL:
	case xoVerticalBindingTop:		return 0;
	case xoVerticalBindingCenter:	return height / 2;
	case xoVerticalBindingBottom:	return height;
	case xoVerticalBindingBaseline:
		if ( IsDefined(baseline) )
			return baseline;
		else
		{
			XOTRACE_LAYOUT_WARNING( "Undefined baseline used in alignment\n" );
			return height;
		}
	default:
		XOASSERTDEBUG(false);
		return 0;
	}
}

bool xoLayout3::IsSpace( int ch )
{
	return ch == 32;
}

bool xoLayout3::IsLinebreak( int ch )
{
	return ch == '\r' || ch == '\n';
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey( xoRenderDomText* rnode )
{
	uint8 glyphFlags = rnode->IsSubPixel() ? xoGlyphFlag_SubPixel_RGB : 0;
	return xoGlyphCacheKey( rnode->FontID, 0, rnode->FontSizePx, glyphFlags );
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey( const TextRunState& ts )
{
	return MakeGlyphCacheKey( ts.IsSubPixel, ts.FontID, ts.FontSizePx );
}

xoGlyphCacheKey	xoLayout3::MakeGlyphCacheKey( bool isSubPixel, xoFontID fontID, int fontSizePx )
{
	uint8 glyphFlags = isSubPixel ? xoGlyphFlag_SubPixel_RGB : 0;
	return xoGlyphCacheKey( fontID, 0, fontSizePx, glyphFlags );
}

void xoLayout3::FlowNewline( FlowState& flow )
{
	flow.PosMinor = 0;
	flow.PosMajor = flow.MajorMax;
	flow.NumLines++;
}

bool xoLayout3::FlowBreakBefore( const LayoutOutput& cout, FlowState& flow )
{
	if ( cout.GetBreak() == xoBreakBefore )
	{
		FlowNewline( flow );
		return true;
	}
	return false;
}

xoPoint xoLayout3::FlowRun( const LayoutInput& cin, const LayoutOutput& cout, FlowState& flow, xoRenderDomEl* rendEl )
{
	if ( IsDefined(cin.ParentWidth) )
	{
		bool over = flow.PosMinor + cout.NodeWidth > cin.ParentWidth;
		bool futile = flow.PosMinor == 0;
		if ( over && !futile )
			FlowNewline( flow );
	}
	xoPoint offset( flow.PosMinor, flow.PosMajor );
	rendEl->Pos.Offset( offset.X, offset.Y );
	flow.PosMinor += cout.NodeWidth;
	flow.MajorMax = xoMax( flow.MajorMax, flow.PosMajor + cout.NodeHeight );

	if ( cout.GetBreak() == xoBreakAfter )
		FlowNewline( flow );

	return offset;
}
