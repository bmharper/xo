#include "pch.h"
#include "nuRenderDomEl.h"

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
	Style.Discard();
	Children.clear();
}
