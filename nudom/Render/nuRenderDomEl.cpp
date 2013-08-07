#include "pch.h"
#include "nuRenderDomEl.h"
#include "nuRenderStack.h"

nuRenderDomEl::nuRenderDomEl( nuPool* pool )
{
	SetPool( pool );
}

nuRenderDomEl::~nuRenderDomEl()
{
	Discard();
}

void nuRenderDomEl::SetPool( nuPool* pool )
{
	Children.Pool = pool;
}

void nuRenderDomEl::Discard()
{
	InternalID = 0;
	Children.clear();
}

void nuRenderDomEl::SetStyle( nuRenderStack& stack )
{
	auto bgColor = stack.Get( nuCatBackground );
	auto bgImage = stack.Get( nuCatBackgroundImage );
	if ( !bgColor.IsNull() ) Style.BackgroundColor = bgColor.GetColor();
	if ( !bgImage.IsNull() ) Style.BackgroundImageID = bgImage.GetStringID();
}
