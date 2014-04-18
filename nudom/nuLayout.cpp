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
	s.ParentContentBox.SetInt( 0, 0, DocWidth, DocHeight );
	s.PositionedAncestor = s.ParentContentBox;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;

	NUTRACE_LAYOUT( "Layout 3\n" );

	RunNode( s, Doc->Root, &root );
}

void nuLayout::RunNode( NodeState& s, const nuDomNode& node, nuRenderDomNode* rnode )
{
	NUTRACE_LAYOUT( "Layout (%d) Run 1\n", node.GetInternalID() );
	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );
	
	NUTRACE_LAYOUT( "Layout (%d) Run 2\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();

	nuStyleBox margin;
	nuStyleBox padding;
	Stack.GetBox( nuCatMargin_Left, margin );
	Stack.GetBox( nuCatPadding_Left, padding );

	auto width = Stack.Get( nuCatWidth );
	auto height = Stack.Get( nuCatHeight );
	auto borderRadius = Stack.Get( nuCatBorderRadius );
	auto display = Stack.Get( nuCatDisplay );
	auto position = Stack.Get( nuCatPosition );
	auto boxSizing = Stack.Get( nuCatBoxSizing );
	nuPos cwidth = ComputeDimension( s.ParentContentBox.Width(), width.GetSize() );
	nuPos cheight = ComputeDimension( s.ParentContentBox.Height(), height.GetSize() );
	nuPos cborderRadius = ComputeDimension( s.ParentContentBox.Width(), borderRadius.GetSize() );
	nuBox cmargin = ComputeBox( s.ParentContentBox, margin );
	nuBox cpadding = ComputeBox( s.ParentContentBox, padding );
	
	rnode->Style.BorderRadius = nuPosToReal( cborderRadius );

	if ( boxSizing.GetBoxSizing() == nuBoxSizeContent ) {}
	else if ( boxSizing.GetBoxSizing() == nuBoxSizeBorder )
	{
		// TODO: subtract border and padding
	}
	else if ( boxSizing.GetBoxSizing() == nuBoxSizeMargin )
	{
		// TODO: subtract border and padding
		cwidth -= cmargin.Left + cmargin.Right;
		cheight -= cmargin.Top + cmargin.Bottom;
	}

	if ( position.GetPositionType() == nuPositionAbsolute )
	{
		rnode->Pos = ComputeSpecifiedPosition( s );
	}
	else
	{
		rnode->Pos.Left = s.PosX + cmargin.Left;
		rnode->Pos.Top = s.PosY + cmargin.Top;
		rnode->Pos.Right = rnode->Pos.Left + cwidth;
		rnode->Pos.Bottom = rnode->Pos.Top + cheight;
	}

	if ( position.GetPositionType() != nuPositionAbsolute )
	{
		switch( display.GetDisplayType() )
		{
		case nuDisplayInline:
			s.PosX = rnode->Pos.Right + cmargin.Right;
			break;
		case nuDisplayBlock:
			s.PosX = s.ParentContentBox.Left;
			s.PosY = rnode->Pos.Bottom + cmargin.Bottom;
			break;
		}
	}

	if ( position.GetPositionType() == nuPositionRelative )
		ComputeRelativeOffset( s, rnode->Pos );

	NodeState cs = s;
	cs.ParentContentBox = rnode->Pos;
	// 'PositionedAncestor' means the nearest ancestor that was specifically positioned. Same as HTML.
	if ( position.GetPositionType() != nuPositionStatic )	cs.PositionedAncestor = rnode->Pos;
	else													cs.PositionedAncestor = s.ParentContentBox;
	cs.PosX = cs.ParentContentBox.Left;
	cs.PosY = cs.ParentContentBox.Top;

	NUTRACE_LAYOUT( "Layout (%d) Run 3\n", node.GetInternalID() );

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

	Stack.StackPop();
}

void nuLayout::RunText( NodeState& s, const nuDomText& node, nuRenderDomText* rnode )
{
	NUTRACE_LAYOUT( "Layout text (%d) Run 1\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();
	rnode->SetStyle( Stack );

	NUTRACE_LAYOUT( "Layout text (%d) Run 2\n", node.GetInternalID() );

	nuStyleAttrib fontfam = Stack.Get( nuCatFontFamily );
	const char* fontStr = fontfam.GetFont( Doc );

	const nuFont* font = nuGlobal()->FontStore->GetByFacename( fontStr );
	if ( font )
		rnode->FontID = font->ID;
	else
		rnode->FontID = nuGlobal()->FontStore->InsertByFacename( fontStr );

	nuGlyphCache* glyphCache = nuGlobal()->GlyphCache;

	nuStyleAttrib fontSizeAttrib = Stack.Get( nuCatFontSize );
	nuPos fontHeight = ComputeDimension( s.ParentContentBox.Height(), fontSizeAttrib.GetSize() );

	float fontSizePxUnrounded = nuPosToReal( fontHeight );
	
	// round font size to integer units
	rnode->FontSizePx = (uint8) nuRound( fontSizePxUnrounded );

	// Nothing prevents somebody from setting a font size to zero
	if ( rnode->FontSizePx < 1 )
		return;

	nuPos fontHeightRounded = nuRealToPos( rnode->FontSizePx );

	const char* txt = node.GetText();
	intp len = strlen(txt);
	rnode->Text.reserve( len );

	bool subPixel = nuGlobal()->EnableSubpixelText && rnode->FontSizePx <= nuGlobal()->MaxSubpixelGlyphSize;
	uint8 glyphFlags = subPixel ? nuGlyphFlag_SubPixel_RGB : 0;
	if ( subPixel )
		rnode->Flags |= nuRenderDomText::FlagSubPixelGlyphs;

	nuGlyphCacheKey key( rnode->FontID, 0, rnode->FontSizePx, glyphFlags );

	for ( intp i = 0; txt[i]; i++ )
	{
		key.Char = txt[i];
		const nuGlyph* glyph = glyphCache->GetGlyph( key );
		if ( !glyph )
		{
			GlyphsNeeded.insert( key );
			continue;
		}
		if ( glyph->IsNull() )
		{
			// TODO: Handle missing glyph by drawing a rectangle or something
			continue;
		}
		rnode->Text.Count++;
		nuRenderCharEl& rtxt = rnode->Text.back();
		rtxt.Char = key.Char;
		rtxt.X = s.PosX + nuRealx256ToPos(glyph->MetricLeftx256);
		rtxt.Y = s.PosY - nuRealToPos(glyph->MetricTop) + fontHeightRounded;
		s.PosX += nuRealx256ToPos(glyph->MetricLinearHoriAdvancex256);
	}
}

void nuLayout::RenderGlyphsNeeded()
{
	for ( auto it = GlyphsNeeded.begin(); it != GlyphsNeeded.end(); it++ )
		nuGlobal()->GlyphCache->RenderGlyph( *it );
	GlyphsNeeded.clear();
}

nuPos nuLayout::ComputeDimension( nuPos container, nuSize size )
{
	switch ( size.Type )
	{
	case nuSize::NONE: return 0;
	case nuSize::PERCENT: return nuPos((float) container * size.Val * 0.01f);
	case nuSize::PX: return nuRealToPos( size.Val );
	case nuSize::PT: return nuRealToPos( size.Val * PtToPixel );	
	default: NUPANIC("Unrecognized size type"); return 0;
	}
}

nuBox nuLayout::ComputeSpecifiedPosition( const NodeState& s )
{
	nuBox box = s.PositionedAncestor;
	auto left = Stack.Get( nuCatLeft );
	auto right = Stack.Get( nuCatRight );
	auto top = Stack.Get( nuCatTop );
	auto bottom = Stack.Get( nuCatBottom );
	auto width = Stack.Get( nuCatWidth );
	auto height = Stack.Get( nuCatHeight );
	if ( !left.IsNull() )	box.Left = s.PositionedAncestor.Left + ComputeDimension( s.PositionedAncestor.Width(), left.GetSize() );
	if ( !right.IsNull() )	box.Right = s.PositionedAncestor.Right + ComputeDimension( s.PositionedAncestor.Width(), right.GetSize() );
	if ( !top.IsNull() )	box.Top = s.PositionedAncestor.Top + ComputeDimension( s.PositionedAncestor.Height(), top.GetSize() );
	if ( !bottom.IsNull() )	box.Bottom = s.PositionedAncestor.Bottom + ComputeDimension( s.PositionedAncestor.Height(), bottom.GetSize() );
	if ( !width.IsNull() && !left.IsNull() && right.IsNull() ) box.Right = box.Left + ComputeDimension( s.PositionedAncestor.Width(), width.GetSize() );
	if ( !width.IsNull() && left.IsNull() && !right.IsNull() ) box.Left = box.Right - ComputeDimension( s.PositionedAncestor.Width(), width.GetSize() );
	if ( !height.IsNull() && !top.IsNull() && bottom.IsNull() ) box.Bottom = box.Top + ComputeDimension( s.PositionedAncestor.Height(), height.GetSize() );
	if ( !height.IsNull() && top.IsNull() && !bottom.IsNull() ) box.Top = box.Bottom - ComputeDimension( s.PositionedAncestor.Height(), height.GetSize() );
	return box;
}

void nuLayout::ComputeRelativeOffset( const NodeState& s, nuBox& box )
{
	auto left = Stack.Get( nuCatLeft );
	auto top = Stack.Get( nuCatTop );
	if ( !left.IsNull() )		box.Left += ComputeDimension( s.ParentContentBox.Width(), left.GetSize() );
	if ( !top.IsNull() )		box.Top += ComputeDimension( s.ParentContentBox.Height(), top.GetSize() );
}

nuBox nuLayout::ComputeBox( nuBox container, nuStyleBox box )
{
	nuBox b;
	b.Left = ComputeDimension( container.Width(), box.Left );
	b.Right = ComputeDimension( container.Width(), box.Right );
	b.Top = ComputeDimension( container.Height(), box.Top );
	b.Bottom = ComputeDimension( container.Height(), box.Bottom );
	return b;
}
