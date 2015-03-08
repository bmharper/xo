#include "../xo/xo.h"

// This was used when developing the spline rendering code

void Render(xoCanvas2D* canvas, int cx, int cy, float scale);

void xoMain(xoSysWnd* wnd)
{
	int left = 550;
	int width = 700;
	int top = 60;
	int height = 700;
	wnd->SetPosition(xoBox(left, top, left + width, top + height), xoSysWnd::SetPosition_Move | xoSysWnd::SetPosition_Size);

	//auto magic = xoColor::RGBA(0xff, 0xf0, 0xf0, 0xff);

	xoDomCanvas* canvas = wnd->Doc()->Root.AddCanvas();
	canvas->StyleParsef("width: %dep; height: %dep;", width, height);
	canvas->SetImageSizeOnly(width, height);

	canvas->OnMouseMove([](const xoEvent& ev) -> bool {
		xoDomCanvas* canvas = (xoDomCanvas*) ev.Target;
		xoCanvas2D* cx = canvas->GetCanvas2D();
		//Render(cx, (int) ev.Points[0].x, (int) ev.Points[0].y);
		Render(cx, 350, 350, ev.Points[0].x * 0.001f);
		canvas->ReleaseCanvas(cx);
		return true;
	});
}

float Eval(float x, float y)
{
	float v = x * x + y * 10.0f;
	return v * 0.0005f;
}

float Eval2(float x, float y)
{
	float eps = 0.1f;
	float g = Eval(x, y);
	float dx = (Eval(x + eps, y) - Eval(x - eps, y)) / eps;
	float dy = (Eval(x, y + eps) - Eval(x, y - eps)) / eps;
	return g / sqrt(dx * dx + dy * dy);
	//return sqrt(dx * dx + dy * dy);
}

void Render(xoCanvas2D* canvas, int cx, int cy, float scale)
{
	//canvas->Fill(xoColor::White());
	//xoBox box(0, 0, 7, 7);
	//box.Offset(cx - box.Width() / 2.0f, cy - box.Height() / 2.0f);
	//canvas->FillRect(box, xoColor::RGBA(200, 50, 50, 255));
	double start = AbcTimeAccurateRTSeconds();

	for (int y = 0; y < (int) canvas->Height(); y++)
	{
		float my = (float) y - cy;
		xoRGBA* line = (xoRGBA*) canvas->RowPtr(y);
		for (int x = 0; x < (int) canvas->Width(); x++)
		{
			float mx = (float) x - cx;
			//float v = mx * mx + my * 40.0f;
			float xf = (float) (x - cx);
			float yf = (float) (y - cy);
			xf *= scale;
			yf *= scale;
			float v = Eval2(xf, yf);
			v /= scale;
			uint8 r = (uint8) (xoClamp(v, 0.0f, 1.0f) * 255);
			uint8 g = (uint8) (xoClamp(1.0f - v, 0.0f, 1.0f) * 255);
			line[x] = xoRGBA::RGBA(r, g, 0, 255);
		}
	}
	canvas->Invalidate(xoBox(0, 0, canvas->Width(), canvas->Height()));

	XOTRACE("canvas render: %.4f", AbcTimeAccurateRTSeconds() - start);
}