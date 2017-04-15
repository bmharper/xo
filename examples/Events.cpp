#include "../xo/xo.h"

void xoMain(xo::SysWnd* wnd)
{
	xo::Trace("Hello 1\n");
	xo::Doc* doc = wnd->Doc();
	//doc->Root.StyleParse( "margin: 20px;" );
	//doc->Root.StyleParse( "border-radius: 55px;" );
	xo::Trace("Hello 2\n");

	xo::DomNode* blocks[4];
	for (int i = 0; i < 4; i++)
	{
		auto div = doc->Root.AddNode(xo::TagDiv);
		//div->StyleParse( "width: 90px; height: 90px; border-radius: 0px; display: inline;" );
		div->StyleParse(tsf::fmt("width: 90px; height: 90px; border-radius: %vpx; display: inline;", 5 * i + 1).c_str());
		div->StyleParse("margin: 3px;");
		blocks[i] = div;
	}

	// block with text inside it
	auto txtBox = doc->Root.AddNode(xo::TagDiv);
	txtBox->StyleParse("width: 90px; height: 90px; border-radius: 2px; background: #0c0; margin: 3px; position: absolute; font-size: 12ep;");
	txtBox->SetText("This widget spans.. document->textDocument()->activeView()");

	blocks[0]->StyleParse("background: #ff000040");
	blocks[1]->StyleParse("background: #ff000080");
	blocks[2]->StyleParse("background: #ff0000ff");

	xo::Trace("Hello 3\n");

	auto greybox = blocks[3];
	greybox->StyleParse("background: #aaaa; position: absolute; left: 90px; top: 90px;");

	auto onMoveOrTouch = [greybox, txtBox](xo::Event& ev) {
		greybox->StyleParsef("left: %fpx; top: %fpx;", ev.PointsRel[0].x - 45.0, ev.PointsRel[0].y - 45.0);
		txtBox->StyleParsef("left: %fpx; top: %fpx;", ev.PointsRel[0].x * 0.01 + 45.0, ev.PointsRel[0].y * 0.01 + 150.0);
		//txtBox->SetText(ev.IsPressed(xo::Button::MouseRight) ? "right" : "hello");
	};

	doc->Root.OnMouseMove(onMoveOrTouch);
	doc->Root.OnTouch(onMoveOrTouch);

	doc->Root.OnClick([]() {
		xo::Trace("Click\n");
	});

	xo::Trace("Hello 4\n");

	xo::Event inject;
	inject.PointCount = 1;
	inject.PointsRel[0] = xo::VEC2(100,100);
	onMoveOrTouch(inject);

	xo::Trace("Hello 5\n");
}
