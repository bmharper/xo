#include "../xo/xo.h"

using namespace xo;

class Outer : public rx::Control {
public:
	Outer(xo::DomNode* root) : Control(root) {}

	void Render() override {
		Root->AddText("Hello World reac");
	}
};

void xoMain(xo::SysWnd* wnd) {
	//wnd->Doc()->Root.AddText("Hello World reac");
	auto c = new Outer(&wnd->Doc()->Root);
}
