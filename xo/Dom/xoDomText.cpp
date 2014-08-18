#include "pch.h"
#include "nuDomText.h"

nuDomText::nuDomText( nuDoc* doc, nuTag tag ) : nuDomEl(doc, tag)
{
}

nuDomText::~nuDomText()
{
}

void nuDomText::SetText( const char* txt )
{
	Text = txt;
}

const char* nuDomText::GetText() const
{
	return Text.Z;
}

void nuDomText::CloneSlowInto( nuDomEl& c, uint cloneFlags ) const
{
	CloneSlowIntoBase( c, cloneFlags );
	nuDomText& ctext = static_cast<nuDomText&>(c);
	
	ctext.Text = Text;
}

void nuDomText::ForgetChildren()
{
}
