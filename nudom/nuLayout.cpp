#include "pch.h"
#include "nuLayout.h"
#include "nuDoc.h"
#include "Render/nuRenderDomEl.h"
#include "Render/nuStyleResolve.h"
#include "Text/nuFontStore.h"

void nuLayout::Layout( const nuDoc& doc, nuRenderDomEl& root, nuPool* pool )
{
	PtToPixel = 1.0;	// TODO

	NUTRACE_LAYOUT( "Layout 1\n" );

	Doc = &doc;
	Pool = pool;
	Pool->FreeAll();
	root.Children.clear();
	Stack.Initialize( Doc, Pool );

	NUTRACE_LAYOUT( "Layout 2\n" );

	// Add root dummy element to the stack
	//Stack.Stack.add();

	NodeState s;
	s.ParentContentBox.SetInt( 0, 0, Doc->WindowWidth, Doc->WindowHeight );
	s.PositionedAncestor = s.ParentContentBox;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;

	NUTRACE_LAYOUT( "Layout 3\n" );

	Run( s, doc.Root, &root );

	NUTRACE_LAYOUT( "Layout done\n" );
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

#if NU_WIN_DESKTOP
	//const char* zfont = "Microsoft Sans Serif";
	//const char* zfont = "Consolas";
	const char* zfont = "Times New Roman";
#else
	const char* zfont = "Droid Sans";
#endif

	// total hack job
	const nuFont* font = nuGlobal()->FontStore->GetByFacename( nuString(zfont) );
	if ( font )
		rnode->FontID = font->ID;
	else
		rnode->FontID = nuGlobal()->FontStore->InsertByFacename( nuString(zfont) );

	rnode->Style.FontSizePx = 24;

	rnode->Text.resize( node.GetText().Len );
	const char* txt = node.GetText().Z;

	for ( intp i = 0; txt[i]; i++ )
	{
		rnode->Text[i].Char = txt[i];
		rnode->Text[i].X = s.PosX + nuRealToPos(10);
		rnode->Text[i].Y = s.PosY + nuRealToPos(10);
		s.PosX += nuRealToPos( 21 );
	}
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
