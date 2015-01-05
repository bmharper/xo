#include "../xo/xo.h"

void xoMain(xoSysWnd* wnd)
{
	xoDoc* doc = wnd->Doc();
	xoDomCanvas* canvasEl = doc->Root.AddCanvas();
	canvasEl->SetSize(400, 300);
	canvasEl->SetText("Hello blank canvas");
	canvasEl->StyleParse("font-size: 14ep");
	canvasEl->StyleParse("color: #080");
	//canvasEl->StyleParse( "background: #f00" );
	xoCanvas2D* c2d = canvasEl->GetCanvas2D();
	Vec2f vx[] = {
		{20,25},
		{60,25},
		{40,50},
	};
	c2d->Fill(xoColor::RGBA(0,0,0,0));
	c2d->StrokeLine(true, arraysize(vx), &vx[0].x, sizeof(vx[0]), xoColor::RGBA(200, 0, 0, 255), 5.0f);
	delete c2d;
}
