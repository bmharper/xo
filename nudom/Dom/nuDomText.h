#pragma once
#include "nuDomEl.h"

class NUAPI nuDomText : public nuDomEl
{
	DISALLOW_COPY_AND_ASSIGN(nuDomText);
public:
					nuDomText( nuDoc* doc, nuTag tag );
					virtual ~nuDomText();

	virtual void			SetText( const char* txt ) override;
	virtual const char*		GetText() const override;
	virtual void			CloneSlowInto( nuDomEl& c, uint cloneFlags ) const override;
	virtual void			ForgetChildren() override;

protected:
	nuString				Text;			// Applicable only to nuTagText elements
};