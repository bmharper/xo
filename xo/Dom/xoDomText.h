#pragma once
#include "xoDomEl.h"

class XOAPI xoDomText : public xoDomEl
{
	DISALLOW_COPY_AND_ASSIGN(xoDomText);
public:
					xoDomText( xoDoc* doc, xoTag tag );
					virtual ~xoDomText();

	virtual void			SetText( const char* txt ) override;
	virtual const char*		GetText() const override;
	virtual void			CloneSlowInto( xoDomEl& c, uint cloneFlags ) const override;
	virtual void			ForgetChildren() override;

protected:
	xoString				Text;			// Applicable only to xoTagText elements
};