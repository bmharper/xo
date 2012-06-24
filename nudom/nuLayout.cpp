#include "pch.h"
#include "nuLayout.h"
#include "nuDoc.h"

void nuLayout::Reset()
{
	DefaultWidth.SetSize( nuCatWidth, nuSize::Percent(100) );
	DefaultHeight.SetSize( nuCatHeight, nuSize::Percent(100) );
	DefaultPadding.SetBox( nuCatPadding, nuStyleBox::MakeZero() );
	DefaultMargin.SetBox( nuCatPadding, nuStyleBox::MakeZero() );
	DefaultBorderRadius.SetSize( nuCatBorderRadius, nuSize::Pixels(0) );
	DefaultDisplay.SetDisplay( nuDisplayInline );
	DefaultPosition.SetPosition( nuPositionStatic );
}

void nuLayout::Layout( const nuDoc& doc, nuRenderDomEl& root, nuPool* pool )
{
	Reset();

	PtToPixel = 1.0;	// TODO

	Doc = &doc;
	Pool = pool;
	Pool->FreeAll();
	root.Children.clear();

	NodeState s;
	s.ParentContentBox.SetInt( 0, 0, Doc->WindowWidth, Doc->WindowHeight );
	s.PositionedAncestor = s.ParentContentBox;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;

	Run( s, doc.Root, &root );
}

void nuLayout::Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode )
{
	nuStyle& style = rnode->Style;
	style.Discard();
	style.Compute( *Doc, node );

	rnode->InternalID = node.InternalID;

	auto width = style.Get( nuCatWidth );
	auto height = style.Get( nuCatHeight );
	auto margin = style.Get( nuCatMargin );
	auto padding = style.Get( nuCatPadding );
	auto borderRadius = style.Get( nuCatBorderRadius );
	auto display = style.Get( nuCatDisplay );
	auto position = style.Get( nuCatPosition );
	if ( !width ) width = &DefaultWidth;
	if ( !height ) height = &DefaultHeight;
	if ( !margin ) margin = &DefaultMargin;
	if ( !padding ) padding = &DefaultPadding;
	if ( !borderRadius ) borderRadius = &DefaultBorderRadius;
	if ( !display ) display = &DefaultDisplay;
	if ( !position ) position = &DefaultPosition;
	nuPos cwidth = ComputeDimension( s.ParentContentBox.Width(), width->Size );
	nuPos cheight = ComputeDimension( s.ParentContentBox.Height(), height->Size );
	nuPos cborderRadius = ComputeDimension( s.ParentContentBox.Width(), borderRadius->Size );
	nuBox cmargin = ComputeBox( s.ParentContentBox, margin->Box );
	nuBox cpadding = ComputeBox( s.ParentContentBox, padding->Box );
	
	rnode->BorderRadius = nuPosToReal( cborderRadius );

	if ( position->Position == nuPositionAbsolute )
	{
		rnode->Pos = ComputeSpecifiedPosition( s, style );
	}
	else
	{
		rnode->Pos.Left = s.PosX + cmargin.Left;
		rnode->Pos.Top = s.PosY + cmargin.Top;
		rnode->Pos.Right = rnode->Pos.Left + cwidth;
		rnode->Pos.Bottom = rnode->Pos.Top + cheight;
	}

	if ( position->Position != nuPositionAbsolute )
	{
		switch( display->Display )
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

	if ( position->Position == nuPositionRelative )
		ComputeRelativeOffset( s, style, rnode->Pos );

	NodeState cs = s;
	cs.ParentContentBox = rnode->Pos;
	// 'PositionedAncestor' means the nearest ancestor that was specifically positioned. Same as HTML.
	if ( position->Position != nuPositionStatic )	cs.PositionedAncestor = rnode->Pos;
	else											cs.PositionedAncestor = s.ParentContentBox;
	cs.PosX = cs.ParentContentBox.Left;
	cs.PosY = cs.ParentContentBox.Top;

	const auto& nodeChildren = node.GetChildren();
	for ( int i = 0; i < nodeChildren.size(); i++ )
	{
		nuRenderDomEl* rchild = Pool->AllocT<nuRenderDomEl>( true );
		rchild->SetPool( Pool );
		rnode->Children += rchild;
		Run( cs, *nodeChildren[i], rchild );
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

nuBox nuLayout::ComputeSpecifiedPosition( const NodeState& s, const nuStyle& style )
{
	nuBox box = s.PositionedAncestor;
	auto left = style.Get( nuCatLeft );
	auto right = style.Get( nuCatRight );
	auto top = style.Get( nuCatTop );
	auto bottom = style.Get( nuCatBottom );
	auto width = style.Get( nuCatWidth );
	auto height = style.Get( nuCatHeight );
	if ( left )		box.Left = s.PositionedAncestor.Left + ComputeDimension( s.PositionedAncestor.Width(), left->Size );
	if ( right )	box.Right = s.PositionedAncestor.Right + ComputeDimension( s.PositionedAncestor.Width(), right->Size );
	if ( top )		box.Top = s.PositionedAncestor.Top + ComputeDimension( s.PositionedAncestor.Height(), top->Size );
	if ( bottom )	box.Bottom = s.PositionedAncestor.Bottom + ComputeDimension( s.PositionedAncestor.Height(), bottom->Size );
	if ( width && left && !right ) box.Right = box.Left + ComputeDimension( s.PositionedAncestor.Width(), width->Size );
	if ( width && !left && right ) box.Left = box.Right - ComputeDimension( s.PositionedAncestor.Width(), width->Size );
	if ( height && top && !bottom ) box.Bottom = box.Top + ComputeDimension( s.PositionedAncestor.Height(), height->Size );
	if ( height && !top && bottom ) box.Top = box.Bottom + ComputeDimension( s.PositionedAncestor.Height(), height->Size );
	return box;
}

void nuLayout::ComputeRelativeOffset( const NodeState& s, const nuStyle& style, nuBox& box )
{
	auto left = style.Get( nuCatLeft );
	auto top = style.Get( nuCatTop );
	if ( left )		box.Left += ComputeDimension( s.ParentContentBox.Width(), left->Size );
	if ( top )		box.Top += ComputeDimension( s.ParentContentBox.Height(), top->Size );
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
