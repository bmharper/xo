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

void MsgBox::Show(Doc* doc, const char* msg) {
	auto grabber = doc->Root.AddNode(xo::TagDiv);
	grabber->AddClass("xo.msgbox.grabber");

	auto box = (DomNode*) grabber->ParseAppend("<div class='xo.msgbox.box'></div>");
	auto lab = (DomNode*) box->ParseAppend("<lab class='xo.msgbox.txt'></lab>");
	lab->SetText(msg);
	//auto okBtn = (DomNode*) box->ParseAppend("<lab class='xo.button'>OK</lab>");
	auto okBtn = (DomNode*) Button::AppendTo(box, "OK");

	okBtn->OnClick([doc, grabber](Event& ev) {
		doc->Root.DeleteChild(grabber);
	});
}
}
}
