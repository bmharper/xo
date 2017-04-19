#include "../xo/xo.h"

void xoMain(xo::SysWnd* wnd) {
	xo::Doc*       doc      = wnd->Doc();
	xo::DomCanvas* canvasEl = doc->Root.AddCanvas();
	canvasEl->SetSize(256 + 40, 256 + 40);
	canvasEl->SetText("Hello blank canvas");
	canvasEl->StyleParse("margin: 10ep; border: 1px #000");
	canvasEl->StyleParse("font-size: 14ep");
	canvasEl->StyleParse("color: #080");
	//canvasEl->StyleParse("background: #000");
	//canvasEl->StyleParse("background: #fff");

	canvasEl->OnMouseMove([canvasEl](xo::Event& ev) {
		int xc = (int) ev.PointsRel[0].x - 20;
		int yc = (int) ev.PointsRel[0].y - 20;

		xc = xo::Clamp(xc, 0, 255);
		yc = xo::Clamp(yc, 0, 255);

		xo::Canvas2D* c2d  = canvasEl->GetCanvas2D();
		Vec2f         vx[] = {
            {20, 25},
            {60, 25},
            {40, 50},
        };
		//c2d->Fill(xo::Color::RGBA(0, 0, 0, 0));
		//c2d->Fill(xo::Color::RGBA(255,255,255,255));
		c2d->Fill(xo::Color::RGBA(255,255,255,xc));
		c2d->StrokeLine(true, arraysize(vx), &vx[0].x, sizeof(vx[0]), xo::Color::RGBA(200, 0, 0, 255), 5.0f);
		c2d->StrokeCircle(80, 80, 5, xo::Color::RGBA(235, 0, 0, 200), 1.5f);
		c2d->StrokeCircle(80, 80, 15, xo::Color::RGBA(255, 0, 0, 200), 3.0f);

		for (int x = 0; x < 255; x++) {
			// To make this demo work, you must comment out "c = c.Premultiply()" inside Canvas2D::ColorToAggS8,
			// otherwise everything is double-premultiplied.
			auto c1 = xo::Color::RGBA(255, 0, 0, x);
			auto c2 = c1.Premultiply();
			auto c3 = c1.PremultiplySRGB();
			auto c4 = c1.PremultiplyTweaked();
			c2d->StrokeLine(x + 0.5f, 100, x + 0.5f, 120, c1, 1.0f);
			c2d->StrokeLine(x + 0.5f, 120, x + 0.5f, 140, c2, 1.0f);
			c2d->StrokeLine(x + 0.5f, 140, x + 0.5f, 160, c3, 1.0f);
			c2d->StrokeLine(x + 0.5f, 160, x + 0.5f, 180, c4, 1.0f);

			auto raw = c4;
			uint32_t cc = (uint32_t) raw.a << 24 | (uint32_t) raw.b << 16 | (uint32_t) raw.g << 8 | (uint32_t) raw.r;

			uint32_t* px = (uint32_t*) c2d->PixelPtr(x, 190);
			for (int y = 0; y < 20; y++, px += c2d->Stride() / 4) {
				//*px = (uint32_t) x << 24 | x;
				*px = cc;
			}
		}
		auto tex = c2d->GetImage();
		canvasEl->ReleaseAndInvalidate(c2d);

		//canvasEl->StyleParsef("background: #%02x%02x%02x", yc, yc, yc);
	});
}
