#include "pch.h"
#include "nuRenderDomEl.h"
#include "nuRenderStack.h"

nuRenderDomEl::nuRenderDomEl( nuPool* pool )
{
	FontID = nuFontIDNull;
	Char = 0;
	SetPool( pool );
}

nuRenderDomEl::~nuRenderDomEl()
{
	Discard();
}

void nuRenderDomEl::SetPool( nuPool* pool )
{
	Children.Pool = pool;
	Text.Pool = pool;
}

void nuRenderDomEl::Discard()
{
	InternalID = 0;
	Children.clear();
	Text.clear();
}

void nuRenderDomEl::SetStyle( nuRenderStack& stack )
{
	auto bgColor = stack.Get( nuCatBackground );
	auto bgImage = stack.Get( nuCatBackgroundImage );
	if ( !bgColor.IsNull() ) Style.BackgroundColor = bgColor.GetColor();
	if ( !bgImage.IsNull() ) Style.BackgroundImageID = bgImage.GetStringID();
}
