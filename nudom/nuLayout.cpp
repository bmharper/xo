#include "pch.h"
#include "nuLayout.h"
#include "nuDoc.h"
#include "Render/nuRenderDomEl.h"
#include "Render/nuStyleResolve.h"

void nuLayout::Reset()
{
	DefaultWidth.SetSize( nuCatWidth, nuSize::Percent(100) );
	DefaultHeight.SetSize( nuCatHeight, nuSize::Percent(100) );
	DefaultPadding.SetZero();
	DefaultMargin.SetZero();
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
	Stack.Initialize( Doc, Pool );

	// Add root dummy element to the stack
	//Stack.Stack.add();

	NodeState s;
	s.ParentContentBox.SetInt( 0, 0, Doc->WindowWidth, Doc->WindowHeight );
	s.PositionedAncestor = s.ParentContentBox;
	s.PosX = s.ParentContentBox.Left;
	s.PosY = s.ParentContentBox.Top;

	Run( s, doc.Root, &root );
}

void nuLayout::Run( NodeState& s, const nuDomEl& node, nuRenderDomEl* rnode )
{
	if ( node.GetInternalID() == 5 )
		int adsadsa = 23232;

	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );
	
	//nuStyle& style = rnode->Style;
	//style.Discard();
	//style.Compute( *Doc, node );

	rnode->InternalID = node.GetInternalID();

	nuStyleBox margin;
	nuStyleBox padding;
	//nuStyleBox margin = DefaultMargin;
	//nuStyleBox padding = DefaultPadding;
	Stack.GetBox( nuCatMargin_Left, margin );
	Stack.GetBox( nuCatPadding_Left, padding );

	//style.GetBox( nuCatMargin_Left, margin );
	//style.GetBox( nuCatPadding_Left, padding );
	auto width = Stack.Get( nuCatWidth );
	auto height = Stack.Get( nuCatHeight );
	auto borderRadius = Stack.Get( nuCatBorderRadius );
	auto display = Stack.Get( nuCatDisplay );
	auto position = Stack.Get( nuCatPosition );
	//if ( !width ) width = &DefaultWidth;
	//if ( !height ) height = &DefaultHeight;
	//if ( !borderRadius ) borderRadius = &DefaultBorderRadius;
	//if ( !display ) display = &DefaultDisplay;
	//if ( !position ) position = &DefaultPosition;
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

	const auto& nodeChildren = node.GetChildren();
	for ( int i = 0; i < nodeChildren.size(); i++ )
	{
		nuRenderDomEl* rchild = Pool->AllocT<nuRenderDomEl>( true );
		rchild->SetPool( Pool );
		rnode->Children += rchild;
		Run( cs, *nodeChildren[i], rchild );
	}

	Stack.Stack.pop();
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
