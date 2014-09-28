#include "pch.h"
#include "xoDomText.h"

xoDomText::xoDomText( xoDoc* doc, xoTag tag, xoInternalID parentID ) : xoDomEl(doc, tag, parentID)
{
}

xoDomText::~xoDomText()
{
}

void xoDomText::SetText( const char* txt )
{
	Text = txt;
}

const char* xoDomText::GetText() const
{
	return Text.Z;
}

void xoDomText::CloneSlowInto( xoDomEl& c, uint cloneFlags ) const
{
	CloneSlowIntoBase( c, cloneFlags );
	xoDomText& ctext = static_cast<xoDomText&>(c);
	
	ctext.Text = Text;
}

void xoDomText::ForgetChildren()
{
}
