#include "pch.h"
#include "xoRenderStack.h"
#include "xoRenderDoc.h"

void xoRenderStackEl::Reset()
{
	Styles.Reset();
	Pool = NULL;
	HasHoverStyle = false;
	HasFocusStyle = false;
}

xoRenderStackEl& xoRenderStackEl::operator=(const xoRenderStackEl& b)
{
	memcpy(this, &b, sizeof(*this));
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoRenderStack::xoRenderStack()
{
}

xoRenderStack::~xoRenderStack()
{
	delete_all(Stack_Pools);
}

void xoRenderStack::Initialize(const xoDoc* doc, xoPool* pool)
{
	Doc = doc;
	Pool = pool;
	Stack.Pool = pool;

	memset(Defaults, 0, sizeof(Defaults));
	Defaults[xoCatColor].SetColor(xoCatColor, xoColor::RGBA(0,0,0,255));
	Defaults[xoCatDisplay].SetDisplay(xoDisplayInline);
	//Defaults[xoCatBackgroundImage]
	Defaults[xoCatCursor].SetCursor(xoCursorArrow);
	Defaults[xoCatText_Align_Vertical].SetTextAlignVertical(xoTextAlignVerticalBaseline);

	Defaults[xoCatBackColor_Left].SetColor(xoCatBackColor_Left, xoColor::RGBA(0, 0, 0, 0));
	Defaults[xoCatBackColor_Top].SetColor(xoCatBackColor_Top, xoColor::RGBA(0, 0, 0, 0));
	Defaults[xoCatBackColor_Right].SetColor(xoCatBackColor_Right, xoColor::RGBA(0, 0, 0, 0));
	Defaults[xoCatBackColor_Bottom].SetColor(xoCatBackColor_Bottom, xoColor::RGBA(0, 0, 0, 0));

	Defaults[xoCatMargin_Left].SetSize(xoCatMargin_Left, xoSize::Zero());
	Defaults[xoCatMargin_Top].SetSize(xoCatMargin_Top, xoSize::Zero());
	Defaults[xoCatMargin_Right].SetSize(xoCatMargin_Right, xoSize::Zero());
	Defaults[xoCatMargin_Bottom].SetSize(xoCatMargin_Bottom, xoSize::Zero());

	Defaults[xoCatPadding_Left].SetSize(xoCatPadding_Left, xoSize::Zero());
	Defaults[xoCatPadding_Top].SetSize(xoCatPadding_Top, xoSize::Zero());
	Defaults[xoCatPadding_Right].SetSize(xoCatPadding_Right, xoSize::Zero());
	Defaults[xoCatPadding_Bottom].SetSize(xoCatPadding_Bottom, xoSize::Zero());

	Defaults[xoCatBorder_Left].SetSize(xoCatBorder_Left, xoSize::Zero());
	Defaults[xoCatBorder_Top].SetSize(xoCatBorder_Top, xoSize::Zero());
	Defaults[xoCatBorder_Right].SetSize(xoCatBorder_Right, xoSize::Zero());
	Defaults[xoCatBorder_Bottom].SetSize(xoCatBorder_Bottom, xoSize::Zero());

	Defaults[xoCatBorderColor_Left].SetColor(xoCatBorderColor_Left, xoColor::RGBA(0,0,0,255));
	Defaults[xoCatBorderColor_Top].SetColor(xoCatBorderColor_Top, xoColor::RGBA(0,0,0,255));
	Defaults[xoCatBorderColor_Right].SetColor(xoCatBorderColor_Right, xoColor::RGBA(0,0,0,255));
	Defaults[xoCatBorderColor_Bottom].SetColor(xoCatBorderColor_Bottom, xoColor::RGBA(0,0,0,255));

	Defaults[xoCatWidth].SetSize(xoCatWidth, xoSize::Null());
	Defaults[xoCatHeight].SetSize(xoCatHeight, xoSize::Null());
	//Defaults[xoCatTop].SetTop( xoVerticalBindingNULL ); -- do we need to? We don't.
	//Defaults[xoCatLeft]
	//Defaults[xoCatRight]
	//Defaults[xoCatBottom]
	Defaults[xoCatFontSize].SetSize(xoCatFontSize, xoSize::EyePixels(12));
	// Font should inherit from body. xoDoc initializes the default font for xoTagBody
	//Defaults[xoCatFontFamily].SetFont( doc->TagStyles[xoTagBody].Get().GetFont( doc ).Z, doc );
	Defaults[xoCatBorderRadius].SetSize(xoCatBorderRadius, xoSize::Zero());
	Defaults[xoCatPosition].SetPosition(xoPositionStatic);
	Defaults[xoCatFlowContext].SetFlowContext(xoFlowContextNew);
	
	// HTML default is Content. At first I was thinking that our default should be
	// BorderBox, but now I'm not so sure anymore.
	//Defaults[xoCatBoxSizing].SetBoxSizing(xoBoxSizeBorder);	
	Defaults[xoCatBoxSizing].SetBoxSizing(xoBoxSizeContent);

}

void xoRenderStack::Reset()
{
	delete_all(Stack_Pools);
	Stack.clear();
}

xoStyleAttrib xoRenderStack::Get(xoStyleCategories cat) const
{
	xoStyleAttrib v = Stack.back().Styles.Get(cat);
	if (v.IsNull())
		return Defaults[cat];
	else
		return v;
}

void xoRenderStack::GetSizeQuad(xoStyleCategories cat, xoSizeQuad& quad) const
{
	xoStyleCategories base = xoCatMakeBaseQuad(cat);
	quad.Left = Get((xoStyleCategories)(base + 0)).GetSize();
	quad.Top = Get((xoStyleCategories)(base + 1)).GetSize();
	quad.Right = Get((xoStyleCategories)(base + 2)).GetSize();
	quad.Bottom = Get((xoStyleCategories)(base + 3)).GetSize();
}

void xoRenderStack::GetColorQuad(xoStyleCategories cat, xoColorQuad& quad) const
{
	xoStyleCategories base = xoCatMakeBaseQuad(cat);
	quad.Left = Get((xoStyleCategories) (base + 0)).GetColor();
	quad.Top = Get((xoStyleCategories) (base + 1)).GetColor();
	quad.Right = Get((xoStyleCategories) (base + 2)).GetColor();
	quad.Bottom = Get((xoStyleCategories) (base + 3)).GetColor();
}

bool xoRenderStack::HasHoverStyle() const
{
	return Stack.back().HasHoverStyle;
}

bool xoRenderStack::HasFocusStyle() const
{
	return Stack.back().HasFocusStyle;
}

void xoRenderStack::StackPop()
{
	// Stack_Pools never gets downsized
	Stack_Pools.back()->FreeAllExceptOne();
	Stack.pop();
}

xoRenderStackEl& xoRenderStack::StackPush()
{
	xoRenderStackEl& el = Stack.add();
	while (Stack_Pools.size() < Stack.size())
	{
		Stack_Pools += new xoPool();
		Stack_Pools.back()->SetChunkSize(8 * 1024);   // this is mentioned in xoRenderStack docs, so keep that up to date if you change this
	}
	el.Pool = Stack_Pools[Stack.size() - 1];
	return el;
}
