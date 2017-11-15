#include "../xo/xo.h"

using namespace xo;

class Outer : public rx::Control {
public:
	int Foo = 0;

	void Render(std::string& dom) override {
		dom += tsf::fmt("<div>Hello World reactive diffable %v</div>", Foo);
	}

	void OnEvent(const char* evname) override {
		switch (hash::crc32(evname)) {
		case "mdown"_crc32:
			Foo++;
			SetDirty();
			break;
		}
	}
};

void xoMain(xo::SysWnd* wnd) {
	//wnd->Doc()->Root.AddText("Hello World reactive");
	auto c = rx::Control::CreateRoot<Outer>(&wnd->Doc()->Root);
}
