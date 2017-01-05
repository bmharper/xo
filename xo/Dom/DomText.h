#pragma once
#include "DomEl.h"

namespace xo {

class XO_API DomText : public DomEl {
	XO_DISALLOW_COPY_AND_ASSIGN(DomText);

public:
	DomText(xo::Doc* doc, xo::Tag tag, xo::InternalID parentID);
	virtual ~DomText();

	virtual void        SetText(const char* txt) override;
	virtual const char* GetText() const override;
	virtual void        CloneSlowInto(DomEl& c, uint32_t cloneFlags) const override;
	virtual void        ForgetChildren() override;

protected:
	String Text;
};
}
