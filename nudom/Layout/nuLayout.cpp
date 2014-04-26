#include "pch.h"
#include "nuLayout.h"
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
void nuLayout::Layout( const nuDoc& doc, u32 docWidth, u32 docHeight, nuRenderDomNode& root, nuPool* pool )
{
	Doc = &doc;
	DocWidth = docWidth;
	DocHeight = docHeight;
	Pool = pool;
	Stack.Initialize( Doc, Pool );
	Fonts = nuGlobal()->FontStore->GetImmutableTable();

	while ( true )
	{
		LayoutInternal( root );

		if ( GlyphsNeeded.size() == 0 )
		{
			NUTRACE_LAYOUT( "Layout done\n" );
			break;
		}
		else
		{
			NUTRACE_LAYOUT( "Layout done (but need another pass for missing glyphs)\n" );
			RenderGlyphsNeeded();
		}
	}
}

void nuLayout::LayoutInternal( nuRenderDomNode& root )
{
	PtToPixel = 1.0;	// TODO

	NUTRACE_LAYOUT( "Layout 1\n" );

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	NUTRACE_LAYOUT( "Layout 2\n" );

	NodeState s;
	memset( &s, 0, sizeof(s) );
	s.ParentContentBox.SetInt( 0, 0, DocWidth, DocHeight );
	//s.PositionedAncestor = s.ParentContentBox;
	s.ParentContentBoxHasWidth = true;
	s.ParentContentBoxHasHeight = true;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;
	s.PosLineX = s.PosX;
	s.PosLineY = s.PosY;

	NUTRACE_LAYOUT( "Layout 3 DocBox = %d,%d,%d,%d\n", s.ParentContentBox.Left, s.ParentContentBox.Top, s.ParentContentBox.Right, s.ParentContentBox.Bottom );

	RunNode( s, Doc->Root, &root );
}

void nuLayout::RunNode( NodeState& s, const nuDomNode& node, nuRenderDomNode* rnode )
{
	NUTRACE_LAYOUT( "Layout (%d) Run 1\n", node.GetInternalID() );
	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );
	
	NUTRACE_LAYOUT( "Layout (%d) Run 2\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();

	//auto display = Stack.Get( nuCatDisplay ).GetDisplayType();
	auto position = Stack.Get( nuCatPosition ).GetPositionType();
	auto boxSizing = Stack.Get( nuCatBoxSizing ).GetBoxSizing();
	nuPos contentWidth = ComputeDimension( s.ParentContentBox.Width(), s.ParentContentBoxHasWidth, nuCatWidth );
	nuPos contentHeight = ComputeDimension( s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, nuCatHeight );
	nuPos borderRadius = ComputeDimension( 0, false, nuCatBorderRadius );
	nuBox margin = ComputeBox( s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, nuCatMargin_Left );
	nuBox padding = ComputeBox( s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, nuCatPadding_Left );
	nuBox border = ComputeBox( s.ParentContentBox, s.ParentContentBoxHasWidth, s.ParentContentBoxHasHeight, nuCatBorder_Left );
	
	// It might make for less arithmetic if we work with borderBoxWidth and borderBoxHeight instead of contentBoxWidth and contentBoxHeight. We'll see.
	bool haveWidth = contentWidth != nuPosNULL;
	bool haveHeight = contentHeight != nuPosNULL;
	rnode->Style.BorderRadius = nuPosToReal( borderRadius );

	if ( boxSizing == nuBoxSizeContent ) {}
	else if ( boxSizing == nuBoxSizeBorder )
	{
		if ( haveWidth )	contentWidth -= border.Left + border.Right + padding.Left + padding.Right;
		if ( haveHeight )	contentHeight -= border.Top + border.Bottom + padding.Top + padding.Bottom;
	}
	else if ( boxSizing == nuBoxSizeMargin )
	{
		if ( haveWidth )	contentWidth -= margin.Left + margin.Right + border.Left + border.Right + padding.Left + padding.Right;
		if ( haveHeight )	contentHeight -= margin.Top + margin.Bottom + border.Top + border.Bottom + padding.Top + padding.Bottom;
	}

	nuBox marginBox;
	if ( position == nuPositionAbsolute )
	{
		marginBox = ComputeSpecifiedPosition( s );
	}
	else
	{
		marginBox.Left = marginBox.Right = s.PosX;
		marginBox.Top = marginBox.Bottom = s.PosY;
		if ( haveWidth ) marginBox.Right += margin.Left + border.Left + padding.Left + contentWidth + padding.Right + border.Right + margin.Right;
		if ( haveHeight ) marginBox.Bottom += margin.Top + border.Top + padding.Top + contentHeight + padding.Bottom + border.Bottom + margin.Bottom;
	}

	/*
	if ( position != nuPositionAbsolute )
	{
		switch( display )
		{
		case nuDisplayInline:
			s.PosX = rnode->Pos.Right + margin.Right;
			break;
		case nuDisplayBlock:
			s.PosX = s.ParentContentBox.Left;
			s.PosY = rnode->Pos.Bottom + margin.Bottom;
			break;
		}
	}
	*/

	nuBox contentBox = marginBox;
	contentBox.Left += border.Left + padding.Left + margin.Left;
	contentBox.Top += border.Top + padding.Top + margin.Top;
	if ( haveWidth )	contentBox.Right -= border.Right + padding.Right + margin.Right;
	else				contentBox.Right = contentBox.Left;
	if ( haveHeight )	contentBox.Bottom -= border.Bottom + padding.Bottom + margin.Bottom;
	else				contentBox.Bottom = contentBox.Top;

	// Check if this block overflows. If we don't have width yet, then we'll check overflow after laying out our children
	if ( position == nuPositionStatic && s.ParentContentBoxHasWidth && haveWidth )
	{
		nuPoint offset = PositionBlock( s, marginBox );
		contentBox.Offset( offset );
	}

	//if ( position == nuPositionRelative ) -- todo
	//	ComputeRelativeOffset( s, marginBox );

	NodeState cs = s;
	cs.ParentContentBox = contentBox;
	cs.ParentContentBoxHasWidth = haveWidth;
	cs.ParentContentBoxHasHeight = haveHeight;
	// 'PositionedAncestor' means the nearest ancestor that was specifically positioned. Same as HTML.
	// But I'm wondering now whether that's a good idea
	//if ( position != nuPositionStatic )	cs.PositionedAncestor = contentBox;
	//else								cs.PositionedAncestor = s.ParentContentBox;
	cs.PosX = contentBox.Left;
	cs.PosY = contentBox.Top;
	cs.PosLineX = cs.PosX;
	cs.PosLineY = cs.PosY;
	cs.PosMaxX = cs.PosX;
	cs.PosMaxY = cs.PosY;

	NUTRACE_LAYOUT( "Layout (%d) Run 3 (position = %d) (%d %d)\n", node.GetInternalID(), (int) position, s.ParentContentBoxHasWidth ? 1 : 0, haveWidth ? 1 : 0 );

	const pvect<nuDomEl*>& nodeChildren = node.GetChildren();
	for ( int i = 0; i < nodeChildren.size(); i++ )
	{
		const nuDomEl* child = nodeChildren[i];
		if ( child->GetTag() == nuTagText )
		{
			nuRenderDomText* rchild = new (Pool->AllocT<nuRenderDomText>(false)) nuRenderDomText( child->GetInternalID(), Pool );
			rnode->Children += rchild;
			RunText( cs, *static_cast<const nuDomText*>(child), rchild );
		}
		else
		{
			nuRenderDomNode* rchild = new (Pool->AllocT<nuRenderDomNode>(false)) nuRenderDomNode( child->GetInternalID(), child->GetTag(), Pool );
			rnode->Children += rchild;
			RunNode( cs, *static_cast<const nuDomNode*>(child), rchild );
		}
	}

	// this is a guess
	if ( !haveWidth )	marginBox.Right = cs.PosMaxX + padding.Right + border.Right + margin.Right;
	if ( !haveHeight )	marginBox.Bottom = cs.PosMaxY + padding.Bottom + border.Bottom + margin.Bottom;

	rnode->Pos = marginBox.ShrunkBy( margin );

	// We couldn't check for overflow until we'd layed out our children. If we do overflow, then we need to retrofit all of our child boxes with an offset.
	if ( position == nuPositionStatic && s.ParentContentBoxHasWidth && !haveWidth )
	{
		nuPoint offset = PositionBlock( s, marginBox );
		if ( offset != nuPoint(0,0) )
			OffsetRecursive( rnode, offset );
	}

	NUTRACE_LAYOUT( "Layout (%d) marginBox: %d,%d,%d,%d\n", node.GetInternalID(), marginBox.Left, marginBox.Top, marginBox.Right, marginBox.Bottom );

	s.PosMaxY = nuMax( s.PosMaxY, marginBox.Bottom );

	Stack.StackPop();
}

void nuLayout::RunText( NodeState& s, const nuDomText& node, nuRenderDomText* rnode )
{
	NUTRACE_LAYOUT( "Layout text (%d) Run 1\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();
	rnode->SetStyle( Stack );

	NUTRACE_LAYOUT( "Layout text (%d) Run 2\n", node.GetInternalID() );

	rnode->FontID = Stack.Get( nuCatFontFamily ).GetFont();

	nuStyleAttrib fontSizeAttrib = Stack.Get( nuCatFontSize );
	nuPos fontHeight = ComputeDimension( s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, fontSizeAttrib.GetSize() );

	float fontSizePxUnrounded = nuPosToReal( fontHeight );
	
	// round font size to integer units
	rnode->FontSizePx = (uint8) nuRound( fontSizePxUnrounded );

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
	GenerateTextWords( s, TempText );
	if ( !TempText.GlyphsNeeded )
		GenerateTextOutput( s, TempText );
}

void nuLayout::GenerateTextWords( NodeState& s, TextRunState& ts )
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
		word.Width += nuRealx256ToPos(glyph->MetricLinearHoriAdvancex256);
	}
}

void nuLayout::GenerateTextOutput( NodeState& s, TextRunState& ts )
{
	const char* txt = ts.Node->GetText();
	nuGlyphCache* glyphCache = nuGlobal()->GlyphCache;
	nuGlyphCacheKey key = MakeGlyphCacheKey( ts.RNode );
	const nuFont* font = Fonts.GetByFontID( ts.RNode->FontID );

	nuPos fontHeightRounded = nuRealToPos( ts.RNode->FontSizePx );
	
	// if we add a "line-height" style then we'll want to multiply that by this
	nuPos lineHeight = nuRealx256ToPos( ts.RNode->FontSizePx * font->NaturalLineHeight_x256 ); 
	if ( nuGlobal()->RoundLineHeights )
		lineHeight = nuPosRoundUp( lineHeight );

	int fontHeightPx = ts.RNode->FontSizePx;
	ts.RNode->Text.reserve( ts.GlyphCount );
	bool parentHasWidth = s.ParentContentBoxHasWidth;
	bool enableKerning = nuGlobal()->EnableKerning;

	for ( intp iword = 0; iword < ts.Words.size(); iword++ )
	{
		const Word& word = ts.Words[iword];
		bool isSpace = word.Length() == 1 && txt[word.Start] == 32;
		bool isNewline = word.Length() == 1 && txt[word.Start] == '\n';
		bool over = parentHasWidth ? s.PosX + word.Width > s.ParentContentBox.Right : false;
		if ( over )
		{
			bool futile = s.PosX == s.ParentContentBox.Left && word.Width > s.ParentContentBox.Width();
			if ( !futile )
			{
				NextLine( s, lineHeight );
				// If the line break was performed for a space, then treat that space as "done"
				if ( isSpace )
					continue;
			}
		}

		if ( isSpace )
		{
			s.PosX += nuRealx256ToPos( font->LinearHoriAdvance_Space_x256 ) * fontHeightPx;
		}
		else if ( isNewline )
		{
			NextLine( s, lineHeight );
		}
		else
		{
			const nuGlyph* prevGlyph = nullptr;
			for ( intp i = word.Start; i < word.End; i++ )
			{
				key.Char = txt[i];
				const nuGlyph* glyph = glyphCache->GetGlyph( key );
				if ( glyph->IsNull() )
					continue;
				if ( enableKerning && prevGlyph )
				{
					FT_Vector kern;
					FT_Get_Kerning( font->FTFace, prevGlyph->FTGlyphIndex, glyph->FTGlyphIndex, FT_KERNING_UNSCALED, &kern );
					nuPos kerning = kern.x * fontHeightPx * 256 / font->FTFace->units_per_EM;
					s.PosX += kerning;
				}
				ts.RNode->Text.Count++;
				nuRenderCharEl& rtxt = ts.RNode->Text.back();
				rtxt.Char = key.Char;
				rtxt.X = s.PosX + nuRealx256ToPos( glyph->MetricLeftx256 );
				rtxt.Y = s.PosY - nuRealToPos( glyph->MetricTop ) + fontHeightRounded;
				s.PosX += nuRealx256ToPos( glyph->MetricLinearHoriAdvancex256 );
				s.PosMaxX = nuMax( s.PosMaxX, s.PosX );
				prevGlyph = glyph;
			}
		}
		s.PosLineY = nuMax( s.PosLineY, s.PosY + lineHeight );
		s.PosMaxY = s.PosLineY;
	}
}

void nuLayout::NextLine( NodeState& s, nuPos textHeight )
{
	// This is similar to PositionBlock, but for text
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.PosLineY;
	s.PosLineY = nuMax( s.PosLineY, s.PosY + textHeight );
	s.PosMaxY = s.PosLineY;
}

nuPoint nuLayout::PositionBlock( NodeState& s, nuBox& marginBox )
{
	NUASSERTDEBUG(s.ParentContentBoxHasWidth);
	
	// Going to next line is futile if this block is as far to the left as possible
	const bool futile = marginBox.Left == s.ParentContentBox.Left;

	nuPoint offset(0,0);
	if ( marginBox.Right <= s.ParentContentBox.Right || futile )
	{
		s.PosX = marginBox.Right;
		s.PosLineX = s.PosX;
		NUTRACE_LAYOUT( "Layout block fits %d,%d\n", s.PosX, s.PosY );
	}
	else
	{
		// Block does not fit on this line, it must move onto the next line.
		// This is like NextLine but without textHeight.
		offset = nuPoint( s.ParentContentBox.Left - marginBox.Left, s.PosMaxY - marginBox.Top );
		marginBox.Offset( offset.X, offset.Y );
		s.PosX = marginBox.Right;
		s.PosY = marginBox.Top;
		s.PosLineX = s.PosX;
		s.PosLineY = s.PosY;
		NUTRACE_LAYOUT( "Layout block does not fit %d,%d\n", s.PosX, s.PosY );
	}
	return offset;
}

void nuLayout::OffsetRecursive( nuRenderDomNode* rnode, nuPoint offset )
{
	rnode->Pos.Offset( offset );
	for ( intp i = 0; i < rnode->Children.size(); i++ )
	{
		if ( rnode->Children[i]->Tag == nuTagText )
		{
			nuRenderDomText* txt = static_cast<nuRenderDomText*>(rnode->Children[i]);
			for ( intp j = 0; j < txt->Text.size(); j++ )
			{
				txt->Text[j].X += offset.X;
				txt->Text[j].Y += offset.Y;
			}
		}
		else
		{
			nuRenderDomNode* node = static_cast<nuRenderDomNode*>(rnode->Children[i]);
			OffsetRecursive( node, offset );
		}
	}
}

bool nuLayout::IsSpace( int ch )
{
	return ch == 32;
}

bool nuLayout::IsLinebreak( int ch )
{
	return ch == '\r' || ch == '\n';
}

nuGlyphCacheKey	nuLayout::MakeGlyphCacheKey( nuRenderDomText* rnode )
{
	uint8 glyphFlags = rnode->IsSubPixel() ? nuGlyphFlag_SubPixel_RGB : 0;
	return nuGlyphCacheKey( rnode->FontID, 0, rnode->FontSizePx, glyphFlags );
}

void nuLayout::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		nuGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

nuPos nuLayout::ComputeDimension( nuPos container, bool isContainerDefined, nuStyleCategories cat )
{
	return ComputeDimension( container, isContainerDefined, Stack.Get( cat ).GetSize() );
}

nuPos nuLayout::ComputeDimension( nuPos container, bool isContainerDefined, nuSize size )
{
	switch ( size.Type )
	{
	case nuSize::NONE: return nuPosNULL;
	case nuSize::PERCENT:
		if ( container == nuPosNULL || !isContainerDefined )
			return nuPosNULL;
		else
			return nuPos((float) container * (size.Val * 0.01f));
	case nuSize::PX: return nuRealToPos( size.Val );
	case nuSize::PT: return nuRealToPos( size.Val * PtToPixel );	
	default: NUPANIC("Unrecognized size type"); return 0;
	}
}

nuBox nuLayout::ComputeSpecifiedPosition( const NodeState& s )
{
	//nuBox reference = s.PositionedAncestor;
	nuBox reference = s.ParentContentBox;
	nuBox box = reference;
	bool refWidthDefined = s.ParentContentBoxHasWidth;
	bool refHeightDefined = s.ParentContentBoxHasHeight;
	auto left = Stack.Get( nuCatLeft );
	auto right = Stack.Get( nuCatRight );
	auto top = Stack.Get( nuCatTop );
	auto bottom = Stack.Get( nuCatBottom );
	auto width = Stack.Get( nuCatWidth );
	auto height = Stack.Get( nuCatHeight );
	if ( !left.IsNull() )	box.Left = reference.Left + ComputeDimension( reference.Width(), refWidthDefined, left.GetSize() );
	if ( !right.IsNull() )	box.Right = reference.Right + ComputeDimension( reference.Width(), refWidthDefined, right.GetSize() );
	if ( !top.IsNull() )	box.Top = reference.Top + ComputeDimension( reference.Height(), refHeightDefined, top.GetSize() );
	if ( !bottom.IsNull() )	box.Bottom = reference.Bottom + ComputeDimension( reference.Height(), refHeightDefined, bottom.GetSize() );
	if ( !width.IsNull() && !left.IsNull() && right.IsNull() ) box.Right = box.Left + ComputeDimension( reference.Width(), refWidthDefined, width.GetSize() );
	if ( !width.IsNull() && left.IsNull() && !right.IsNull() ) box.Left = box.Right - ComputeDimension( reference.Width(), refWidthDefined, width.GetSize() );
	if ( !height.IsNull() && !top.IsNull() && bottom.IsNull() ) box.Bottom = box.Top + ComputeDimension( reference.Height(), refHeightDefined, height.GetSize() );
	if ( !height.IsNull() && top.IsNull() && !bottom.IsNull() ) box.Top = box.Bottom - ComputeDimension( reference.Height(), refHeightDefined, height.GetSize() );
	return box;
}

void nuLayout::ComputeRelativeOffset( const NodeState& s, nuBox& box )
{
	NUTODO;
	//auto left = Stack.Get( nuCatLeft );
	//auto top = Stack.Get( nuCatTop );
	//if ( !left.IsNull() )		box.Left += ComputeDimension( s.ParentContentBox.Width(), s.ParentContentBoxHasWidth, left.GetSize() );
	//if ( !top.IsNull() )		box.Top += ComputeDimension( s.ParentContentBox.Height(), s.ParentContentBoxHasHeight, top.GetSize() );
}

nuBox nuLayout::ComputeBox( nuBox container, bool widthDefined, bool heightDefined, nuStyleCategories cat )
{
	return ComputeBox( container, widthDefined, heightDefined, Stack.GetBox( cat ) );
}

nuBox nuLayout::ComputeBox( nuBox container, bool widthDefined, bool heightDefined, nuStyleBox box )
{
	nuBox b;
	b.Left = ComputeDimension( container.Width(), widthDefined, box.Left );
	b.Right = ComputeDimension( container.Width(), widthDefined, box.Right );
	b.Top = ComputeDimension( container.Height(), heightDefined, box.Top );
	b.Bottom = ComputeDimension( container.Height(), heightDefined, box.Bottom );
	return b;
}
