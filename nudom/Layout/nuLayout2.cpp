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
			//RenderGlyphsNeeded();
		}
	}
}

void nuLayout2::LayoutInternal( nuRenderDomNode& root )
{
	PtToPixel = 1.0;	// TODO
	EpToPixel = nuGlobal()->EpToPixel;

	NUTRACE_LAYOUT( "Layout 1\n" );

	Pool->FreeAll();
	root.Children.clear();
	Stack.Reset();

	NUTRACE_LAYOUT( "Layout 2\n" );

	LayoutInput in;
	in.ParentWidth = nuIntToPos( DocWidth );
	in.ParentHeight = nuIntToPos( DocHeight );
	in.ParentBaseline = nuPosNULL;

	LayoutOutput out;

	NUTRACE_LAYOUT( "Layout 3 DocBox = %d,%d,%d,%d\n", s.ParentContentBox.Left, s.ParentContentBox.Top, s.ParentContentBox.Right, s.ParentContentBox.Bottom );

	RunNode( Doc->Root, in, out, &root );
}

void nuLayout2::RunNode( const nuDomNode& node, const LayoutInput& in, LayoutOutput& out, nuRenderDomNode* rnode )
{
	NUTRACE_LAYOUT( "Layout (%d) Run 1\n", node.GetInternalID() );
	nuStyleResolver::ResolveAndPush( Stack, &node );
	rnode->SetStyle( Stack );
	
	NUTRACE_LAYOUT( "Layout (%d) Run 2\n", node.GetInternalID() );
	rnode->InternalID = node.GetInternalID();

	nuBoxSizeType boxSizing = Stack.Get( nuCatBoxSizing ).GetBoxSizing();
	nuPos contentWidth = ComputeDimension( in.ParentWidth, nuCatWidth );
	nuPos contentHeight = ComputeDimension( in.ParentHeight, nuCatHeight );
	nuBox margin = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatMargin_Left );		// it may be wise to disallow percentage sizing here
	nuBox padding = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatPadding_Left );	// same here
	nuBox border = ComputeBox( in.ParentWidth, in.ParentHeight, nuCatBorder_Left );		// and here
	
	// It might make for less arithmetic if we work with marginBoxWidth and marginBoxHeight instead of contentBoxWidth and contentBoxHeight. We'll see.

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

	for ( intp i = 0; i < node.ChildCount(); i++ )
	{
		const nuDomEl* c = node.ChildByIndex( i );
		LayoutInput cin;
		LayoutOutput cout;
		cin.ParentBaseline = in.ParentBaseline;
		cin.ParentWidth = contentWidth;
		cin.ParentHeight = contentHeight;
		if ( c->GetTag() == nuTagText )
		{
		}
		else
		{
			nuRenderDomNode* rchild = new (Pool->AllocT<nuRenderDomNode>(false)) nuRenderDomNode( c->GetInternalID(), c->GetTag(), Pool );
			rnode->Children += rchild;
			RunNode( *static_cast<const nuDomNode*>(c), cin, cout, rchild );
			PositionChild( cin, cout, rchild );
		}
	}

	rnode->Pos = nuBox( 0, 0, 0, 0 );
	if ( IsDefined(contentWidth) ) rnode->Pos.Right = contentWidth;
	if ( IsDefined(contentHeight) ) rnode->Pos.Bottom = contentHeight;
	rnode->Style.BackgroundColor = Stack.Get( nuCatBackground ).GetColor();
	rnode->Style.BorderRadius = 0;

	out.NodeBaseline = nuPosNULL;
	out.NodeWidth = contentWidth;
	out.NodeHeight = contentHeight;
	out.Binds = ComputeBinds();

	Stack.StackPop();
}

void nuLayout2::PositionChild( const LayoutInput& cin, const LayoutOutput& cout, nuRenderDomNode* rchild )
{
	nuPoint offset(0,0);
	switch ( cout.Binds.HChild )
	{
	case nuHorizontalBindingCenter:
		switch ( cout.Binds.HParent )
		{
		case nuHorizontalBindingCenter:
			offset.X = (cin.ParentWidth - cout.NodeWidth) / 2;
			break;
		}
		break;
	}
	switch ( cout.Binds.VChild )
	{
	case nuVerticalBindingCenter:
		switch ( cout.Binds.VParent )
		{
		case nuVerticalBindingCenter:
			offset.Y = (cin.ParentHeight - cout.NodeHeight) / 2;
			break;
		}
		break;
	}
	if ( offset != nuPoint(0,0) )
	{
		OffsetRecursive( rchild, offset );
	}
}

void nuLayout2::OffsetRecursive( nuRenderDomNode* rnode, nuPoint offset )
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

	BindingSet binds = {0};

	if ( left != nuHorizontalBindingNULL )		{ binds.HChild = nuHorizontalBindingLeft; binds.HParent = left; }
	if ( hcenter != nuHorizontalBindingNULL )	{ binds.HChild = nuHorizontalBindingCenter; binds.HParent = hcenter; }
	if ( right != nuHorizontalBindingNULL )		{ binds.HChild = nuHorizontalBindingRight; binds.HParent = right; }

	if ( top != nuVerticalBindingNULL )			{ binds.VChild = nuVerticalBindingTop; binds.VParent = top; }
	if ( vcenter != nuVerticalBindingNULL )		{ binds.VChild = nuVerticalBindingCenter; binds.VParent = vcenter; }
	if ( bottom != nuVerticalBindingNULL )		{ binds.VChild = nuVerticalBindingBottom; binds.VParent = bottom; }
	if ( baseline != nuVerticalBindingNULL )	{ binds.VChild = nuVerticalBindingBaseline; binds.VParent = baseline; }

	return binds;
}
