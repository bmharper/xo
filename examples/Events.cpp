#include "../xo/xo.h"

void xoMain(xoSysWnd* wnd)
{
	XOTRACE("Hello 1\n");
	xoDoc* doc = wnd->Doc();
	//doc->Root.StyleParse( "margin: 20px;" );
	//doc->Root.StyleParse( "border-radius: 55px;" );
	XOTRACE("Hello 2\n");

	xoDomNode* blocks[4];
	for (int i = 0; i < 4; i++)
	{
		xoDomNode* div = doc->Root.AddNode(xoTagDiv);
		//div->StyleParse( "width: 90px; height: 90px; border-radius: 0px; display: inline;" );
		div->StyleParse(xo::fmt("width: 90px; height: 90px; border-radius: %vpx; display: inline;", 5 * i + 1).Z);
		div->StyleParse("margin: 3px;");
		blocks[i] = div;
	}

	// block with text inside it
	xoDomNode* txtBox = doc->Root.AddNode(xoTagDiv);
	txtBox->StyleParse("width: 90px; height: 90px; border-radius: 2px; background: #0c0; margin: 3px; position: absolute; font-size: 12ep;");
	txtBox->SetText("This widget spans.. document->textDocument()->activeView()");

	blocks[0]->StyleParse("background: #ff000040");
	blocks[1]->StyleParse("background: #ff000080");
	blocks[2]->StyleParse("background: #ff0000ff");

	XOTRACE("Hello 3\n");

	xoDomNode* greybox = blocks[3];
	greybox->StyleParse("background: #aaaa; position: absolute; left: 90px; top: 90px;");

	auto onMoveOrTouch = [greybox, txtBox](const xoEvent& ev) -> bool {
		greybox->StyleParsef("left: %fpx; top: %fpx;", ev.Points[0].x - 45.0, ev.Points[0].y - 45.0);
		txtBox->StyleParsef("left: %fpx; top: %fpx;", ev.Points[0].x * 0.01 + 45.0, ev.Points[0].y * 0.01 + 150.0);
		return true;
	};
	doc->Root.OnMouseMove(onMoveOrTouch);
	doc->Root.OnTouch(onMoveOrTouch);

	XOTRACE("Hello 4\n");

	xoEvent inject;
	inject.PointCount = 1;
	inject.Points[0] = XOVEC2(100,100);
	onMoveOrTouch(inject);

	XOTRACE("Hello 5\n");
}
