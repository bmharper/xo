#include "pch.h"
#include "nuRenderStack.h"

void nuRenderStack::Initialize( const nuDoc* doc, nuPool* pool )
{
	Doc = doc;
	Pool = pool;
	Stack.Pool = pool;

	/*
	DefaultWidth.SetSize( nuCatWidth, nuSize::Percent(100) );
	DefaultHeight.SetSize( nuCatHeight, nuSize::Percent(100) );
	DefaultPadding.SetZero();
	DefaultMargin.SetZero();
	DefaultBorderRadius.SetSize( nuCatBorderRadius, nuSize::Pixels(0) );
	DefaultDisplay.SetDisplay( nuDisplayInline );
	DefaultPosition.SetPosition( nuPositionStatic );
	*/
	//Defaults[nuCatNULL] = 
	memset( Defaults, 0, sizeof(Defaults) );
	Defaults[nuCatColor].SetColor( nuCatColor, nuColor::RGBA(0,0,0,255) );
	Defaults[nuCatDisplay].SetDisplay( nuDisplayInline );
	Defaults[nuCatBackground].SetColor( nuCatBackground, nuColor::RGBA(0,0,0,0) );
	//Defaults[nuCatBackgroundImage]
	//Defaults[nuCatDummy1_UseMe]
	//Defaults[nuCatDummy2_UseMe]
	//Defaults[nuCatDummy3_UseMe]
	Defaults[nuCatMargin_Left].SetSize( nuCatMargin_Left, nuSize::Zero() );
	Defaults[nuCatMargin_Top].SetSize( nuCatMargin_Top, nuSize::Zero() );
	Defaults[nuCatMargin_Right].SetSize( nuCatMargin_Right, nuSize::Zero() );
	Defaults[nuCatMargin_Bottom].SetSize( nuCatMargin_Bottom, nuSize::Zero() );
	Defaults[nuCatPadding_Left].SetSize( nuCatPadding_Left, nuSize::Zero() );
	Defaults[nuCatPadding_Top].SetSize( nuCatPadding_Top, nuSize::Zero() );
	Defaults[nuCatPadding_Right].SetSize( nuCatPadding_Right, nuSize::Zero() );
	Defaults[nuCatPadding_Bottom].SetSize( nuCatPadding_Bottom, nuSize::Zero() );
	Defaults[nuCatBorder_Left].SetSize( nuCatBorder_Left, nuSize::Zero() );
	Defaults[nuCatBorder_Top].SetSize( nuCatBorder_Top, nuSize::Zero() );
	Defaults[nuCatBorder_Right].SetSize( nuCatBorder_Right, nuSize::Zero() );
	Defaults[nuCatBorder_Bottom].SetSize( nuCatBorder_Bottom, nuSize::Zero() );
	//Defaults[nuCatWidth]
	//Defaults[nuCatHeight]
	//Defaults[nuCatTop]
	//Defaults[nuCatLeft]
	//Defaults[nuCatRight]
	//Defaults[nuCatBottom]
	Defaults[nuCatFontSize].SetSize( nuCatFontSize, nuSize::Points(10) );
	//Defaults[nuCatFontFamily]
	Defaults[nuCatBorderRadius].SetSize( nuCatBorderRadius, nuSize::Zero() );
	Defaults[nuCatPosition].SetPosition( nuPositionStatic );
	//XY(END)
}

nuStyleAttrib nuRenderStack::Get( nuStyleCategories cat ) const
{
	nuStyleAttrib v = Stack.back().Styles.Get( cat );
	if ( v.IsNull() )
		return Defaults[cat];
	else
		return v;
}

void nuRenderStack::GetBox( nuStyleCategories cat, nuStyleBox& box ) const
{
	nuStyleCategories base = nuCatMakeBaseBox(cat);
	box.Left = Get( (nuStyleCategories) (base + 0) ).GetSize();
	box.Top = Get( (nuStyleCategories) (base + 1) ).GetSize();
	box.Right = Get( (nuStyleCategories) (base + 2) ).GetSize();
	box.Bottom = Get( (nuStyleCategories) (base + 3) ).GetSize();
}
