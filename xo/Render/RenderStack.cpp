#include "pch.h"
#include "RenderStack.h"
#include "RenderDoc.h"

namespace xo {

void RenderStackEl::Reset() {
	Styles.Reset();
	Pool          = NULL;
	HasHoverStyle = false;
	HasFocusStyle = false;
}

RenderStackEl& RenderStackEl::operator=(const RenderStackEl& b) {
	memcpy(this, &b, sizeof(*this));
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderStack::RenderStack() {
}

RenderStack::~RenderStack() {
	DeleteAll(Stack_Pools);
}

void RenderStack::Initialize(const xo::Doc* doc, xo::Pool* pool) {
	Doc        = doc;
	Pool       = pool;
	Stack.Pool = pool;

	memset(Defaults, 0, sizeof(Defaults));
	Defaults[CatColor].SetColor(CatColor, Color::RGBA(0, 0, 0, 255));
	Defaults[CatDisplay].SetDisplay(DisplayInline);
	Defaults[CatBackground].SetColor(CatBackground, Color::RGBA(0, 0, 0, 0));
	//Defaults[CatBackgroundImage]
	Defaults[CatCursor].SetCursor(CursorArrow);
	Defaults[CatText_Align_Vertical].SetTextAlignVertical(TextAlignVerticalBaseline);
	//Defaults[CatDummy2_UseMe]
	//Defaults[CatDummy3_UseMe]
	Defaults[CatMargin_Left].SetSize(CatMargin_Left, Size::Zero());
	Defaults[CatMargin_Top].SetSize(CatMargin_Top, Size::Zero());
	Defaults[CatMargin_Right].SetSize(CatMargin_Right, Size::Zero());
	Defaults[CatMargin_Bottom].SetSize(CatMargin_Bottom, Size::Zero());

	Defaults[CatPadding_Left].SetSize(CatPadding_Left, Size::Zero());
	Defaults[CatPadding_Top].SetSize(CatPadding_Top, Size::Zero());
	Defaults[CatPadding_Right].SetSize(CatPadding_Right, Size::Zero());
	Defaults[CatPadding_Bottom].SetSize(CatPadding_Bottom, Size::Zero());

	Defaults[CatBorder_Left].SetSize(CatBorder_Left, Size::Zero());
	Defaults[CatBorder_Top].SetSize(CatBorder_Top, Size::Zero());
	Defaults[CatBorder_Right].SetSize(CatBorder_Right, Size::Zero());
	Defaults[CatBorder_Bottom].SetSize(CatBorder_Bottom, Size::Zero());

	Defaults[CatBorderColor_Left].SetColor(CatBorderColor_Left, Color::RGBA(0, 0, 0, 255));
	Defaults[CatBorderColor_Top].SetColor(CatBorderColor_Top, Color::RGBA(0, 0, 0, 255));
	Defaults[CatBorderColor_Right].SetColor(CatBorderColor_Right, Color::RGBA(0, 0, 0, 255));
	Defaults[CatBorderColor_Bottom].SetColor(CatBorderColor_Bottom, Color::RGBA(0, 0, 0, 255));

	Defaults[CatWidth].SetSize(CatWidth, Size::Null());
	Defaults[CatHeight].SetSize(CatHeight, Size::Null());
	//Defaults[CatTop].SetTop( VerticalBindingNULL ); -- do we need to? We don't.
	//Defaults[CatLeft]
	//Defaults[CatRight]
	//Defaults[CatBottom]
	Defaults[CatFontSize].SetSize(CatFontSize, Size::EyePixels(12));
	// Font should inherit from body. Doc initializes the default font for TagBody
	//Defaults[CatFontFamily].SetFont( doc->TagStyles[TagBody].Get().GetFont( doc ).Z, doc );
	Defaults[CatBorderRadius].SetSize(CatBorderRadius, Size::Zero());
	Defaults[CatPosition].SetPosition(PositionStatic);
	Defaults[CatFlowContext].SetFlowContext(FlowContextNew);

	// HTML default is Content. At first I was thinking that our default should be
	// BorderBox, but now I'm not so sure anymore. For example, when you alter your border
	// widths, you don't typically expect your content to be shuffled around.
	//Defaults[CatBoxSizing].SetBoxSizing(BoxSizeBorder);
	Defaults[CatBoxSizing].SetBoxSizing(BoxSizeContent);
}

void RenderStack::Reset() {
	DeleteAll(Stack_Pools);
	Stack.clear();
}

StyleAttrib RenderStack::Get(StyleCategories cat) const {
	StyleAttrib v = Stack.back().Styles.Get(cat);
	if (v.IsNull())
		return Defaults[cat];
	else
		return v;
}

void RenderStack::GetBox(StyleCategories cat, StyleBox& box) const {
	StyleCategories base = CatMakeBaseBox(cat);
	box.Left             = Get((StyleCategories)(base + 0)).GetSize();
	box.Top              = Get((StyleCategories)(base + 1)).GetSize();
	box.Right            = Get((StyleCategories)(base + 2)).GetSize();
	box.Bottom           = Get((StyleCategories)(base + 3)).GetSize();
}

bool RenderStack::HasHoverStyle() const {
	return Stack.back().HasHoverStyle;
}

bool RenderStack::HasFocusStyle() const {
	return Stack.back().HasFocusStyle;
}

void RenderStack::StackPop() {
	// Stack_Pools never gets downsized
	Stack_Pools.back()->FreeAllExceptOne();
	Stack.pop();
}

RenderStackEl& RenderStack::StackPush() {
	RenderStackEl& el = Stack.add();
	while (Stack_Pools.size() < Stack.size()) {
		Stack_Pools += new xo::Pool();
		Stack_Pools.back()->SetChunkSize(8 * 1024); // this is mentioned in RenderStack docs, so keep that up to date if you change this
	}
	el.Pool = Stack_Pools[Stack.size() - 1];
	return el;
}
}
