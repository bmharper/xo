#include "pch.h"
#include "DomText.h"

namespace xo {

DomText::DomText(xo::Doc* doc, xo::Tag tag, xo::InternalID parentID) : DomEl(doc, tag, parentID) {
}

DomText::~DomText() {
}

void DomText::SetText(const char* txt) {
	Text = txt;
	IncVersion();
}

const char* DomText::GetText() const {
	return Text.Z;
}

void DomText::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	CloneSlowIntoBase(c, cloneFlags);
	DomText& ctext = static_cast<DomText&>(c);

	ctext.Text = Text;
}

void DomText::ForgetChildren() {
}
}
