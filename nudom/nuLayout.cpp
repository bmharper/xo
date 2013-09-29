#include "pch.h"
#include "nuLayout.h"
#include "nuDoc.h"
#include "Render/nuRenderDomEl.h"
#include "Render/nuStyleResolve.h"
#include "Text/nuFontStore.h"
#include "Text/nuGlyphCache.h"

/* This is called serially.

Why do we perform layout in multiple passes, loading all missing glyphs at the end of each pass?
The reason is because we eventually want to be able to parallelize layout.

Missing glyphs are a once-off cost, so it's not worth trying to use a mutable glyph cache.

*/
void nuLayout::Layout( const nuDoc& doc, nuRenderDomEl& root, nuPool* pool )
{
	Doc = &doc;
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

void nuLayout::LayoutInternal( nuRenderDomEl& root )
{
	PtToPixel = 1.0;	// TODO

	NUTRACE_LAYOUT( "Layout 1\n" );

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	NUTRACE_LAYOUT( "Layout 2\n" );

	// Add root dummy element to the stack
	//Stack.Stack.add();

	NodeState s;
	s.ParentContentBox.SetInt( 0, 0, Doc->WindowWidth, Doc->WindowHeight );
	s.PositionedAncestor = s.ParentContentBox;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;

	NUTRACE_LAYOUT( "Layout 3\n" );

	Run( s, Doc->Root, &root );
}

void nuLayout::Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode )
{
	NUTRACE_LAYOUT( "Layout (%d) Run 1\n", node.GetInternalID() );
	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );
	
	NUTRACE_LAYOUT( "Layout (%d) Run 2\n", node.GetInternalID() );

	rnode->InternalID = node.GetInternalID();

	if ( node.GetTag() == nuTagText )
	{
		RunText( s, node, rnode );
	}

	nuStyleBox margin;
	nuStyleBox padding;
	Stack.GetBox( nuCatMargin_Left, margin );
	Stack.GetBox( nuCatPadding_Left, padding );

	auto width = Stack.Get( nuCatWidth );
	auto height = Stack.Get( nuCatHeight );
	auto borderRadius = Stack.Get( nuCatBorderRadius );
	auto display = Stack.Get( nuCatDisplay );
	auto position = Stack.Get( nuCatPosition );
	nuPos cwidth = ComputeDimension( s.ParentContentBox.Width(), width.GetSize() );
	nuPos cheight = ComputeDimension( s.ParentContentBox.Height(), height.GetSize() );
	nuPos cborderRadius = ComputeDimension( s.ParentContentBox.Width(), borderRadius.GetSize() );
	nuBox cmargin = ComputeBox( s.ParentContentBox, margin );
	nuBox cpadding = ComputeBox( s.ParentContentBox, padding );
	
	rnode->Style.BorderRadius = nuPosToReal( cborderRadius );

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

	const auto& nodeChildren = node.GetChildren();
	for ( int i = 0; i < nodeChildren.size(); i++ )
	{
		nuRenderDomEl* rchild = Pool->AllocT<nuRenderDomEl>( true );
		rchild->SetPool( Pool );
		rnode->Children += rchild;
		Run( cs, *nodeChildren[i], rchild );
	}

	Stack.StackPop();
}

void nuLayout::RunText( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode )
{
	NUTRACE_LAYOUT( "Layout (%d) Run txt.1\n", node.GetInternalID() );

#if NU_PLATFORM_WIN_DESKTOP
	//const char* zfont = "Microsoft Sans Serif";
	//const char* zfont = "Consolas";
	//const char* zfont = "Times New Roman";
	//const char* zfont = "Verdana";
	const char* zfont = "Tahoma";
#else
	const char* zfont = "Droid Sans";
#endif

	bool subPixel = nuGlobal()->EnableSubpixelText;
	uint8 glyphFlags = subPixel ? nuGlyphFlag_SubPixel_RGB : 0;

	// total hack job
	const nuFont* font = nuGlobal()->FontStore->GetByFacename( nuString(zfont) );
	if ( font )
		rnode->FontID = font->ID;
	else
		rnode->FontID = nuGlobal()->FontStore->InsertByFacename( nuString(zfont) );

	nuGlyphCache* glyphCache = nuGlobal()->GlyphCache;

	float fontSizePx = 12;
	rnode->Style.FontSizePx = (uint8) fontSizePx;

	const nuString& str = node.GetText();
	rnode->Text.reserve( str.Len );
	const char* txt = str.Z;

	nuGlyphCacheKey key( rnode->FontID, 0, rnode->Style.FontSizePx, glyphFlags );

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
		nuRenderTextEl& rtxt = rnode->Text.back();
		rtxt.Char = key.Char;
		rtxt.X = s.PosX + nuRealToPos(glyph->MetricLeftx256 / 256.0f);
		rtxt.Y = s.PosY - nuRealToPos(glyph->MetricTop);
		s.PosX += nuRealToPos(glyph->MetricLinearHoriAdvance * fontSizePx);
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
