#include "pch.h"
#include "MsgBox.h"
#include "Button.h"
#include "../Doc.h"

namespace xo {
namespace controls {

void MsgBox::InitializeStyles(Doc* doc) {
	doc->ClassParse("xo.msgbox.grabber", "position: absolute; hcenter: hcenter; vcenter: vcenter; width: 100%; height: 100%; background: #eee5");
	doc->ClassParse("xo.msgbox.box", "background: #fff; border-radius: 5ep; border: 2ep #889; hcenter: hcenter; vcenter: vcenter; padding: 15ep");
	doc->ClassParse("xo.msgbox.txt", "padding-bottom: 20ep; break: after");
}

void MsgBox::Show(Doc* doc, const char* msg, const char* okMsg) {
	MsgBox* mb = new MsgBox();
	mb->Create(doc, msg, okMsg);
}

void MsgBox::Create(Doc* doc, const char* msg, const char* okMsg) {
	auto grabber = doc->Root.AddNode(xo::TagDiv);
	grabber->AddClass("xo.msgbox.grabber");
	Root = grabber;

	if (!okMsg)
		okMsg = "OK";

	auto box   = (DomNode*) grabber->ParseAppend("<div class='xo.msgbox.box'></div>");
	auto lab   = (DomNode*) box->ParseAppend("<lab class='xo.msgbox.txt'></lab>");
	auto okBtn = (DomNode*) Button::NewText(box, okMsg);
	SetText(msg);

	okBtn->OnClick([this](Event& ev) {
		// When we delete our DOM elements, our lambda gets blown away too.
		// So before we can do Root->Delete(), we must store all state that we need locally.
		auto self = this;
		auto onclose = OnClose;
		Root->Delete();
		delete self;
		if (onclose)
			onclose();
	});
}

void MsgBox::SetText(const char* msg) {
	Root->NodeByIndex(0)->NodeByIndex(0)->SetText(msg);
}

} // namespace controls
} // namespace xo
