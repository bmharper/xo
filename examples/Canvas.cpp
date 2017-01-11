#include "../xo/xo.h"

void xoMain(xo::SysWnd* wnd)
{
	xo::Doc* doc = wnd->Doc();
	xo::DomCanvas* canvasEl = doc->Root.AddCanvas();
	canvasEl->SetSize(400, 300);
	canvasEl->SetText("Hello blank canvas");
	canvasEl->StyleParse("font-size: 14ep");
	canvasEl->StyleParse("color: #080");
	xo::Canvas2D* c2d = canvasEl->GetCanvas2D();
	Vec2f vx[] = {
		{20,25},
		{60,25},
		{40,50},
	};
	c2d->Fill(xo::Color::RGBA(0,0,0,0));
	c2d->StrokeLine(true, arraysize(vx), &vx[0].x, sizeof(vx[0]), xo::Color::RGBA(200, 0, 0, 255), 5.0f);
	c2d->StrokeCircle(80, 80, 5, xo::Color::RGBA(200, 0, 0, 255), 1.0f);
	delete c2d;
}
