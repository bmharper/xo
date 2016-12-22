#include "../xo/xo.h"

#include <omp.h>

// This was used when developing the spline rendering code

void Render(xo::Canvas2D* canvas, int cx, int cy, float scale);

void xoMain(xo::SysWnd* wnd)
{
	int left = 550;
	int width = 1000;
	int top = 60;
	int height = 1000;
	wnd->SetPosition(xo::Box(left, top, left + width, top + height), xo::SysWnd::SetPosition_Move | xo::SysWnd::SetPosition_Size);

	//auto magic = xo::Color::RGBA(0xff, 0xf0, 0xf0, 0xff);

	xo::DomCanvas* canvas = wnd->Doc()->Root.AddCanvas();
	canvas->StyleParsef("width: %dep; height: %dep;", width, height);
	canvas->SetImageSizeOnly(width, height);

	canvas->OnMouseMove([width, height](const xo::Event& ev) -> bool {
		xo::DomCanvas* canvas = (xo::DomCanvas*) ev.Target;
		xo::Canvas2D* cx = canvas->GetCanvas2D();
		//Render(cx, (int) ev.Points[0].x, (int) ev.Points[0].y);
		Render(cx, width / 2, height / 2, FLT_EPSILON + ev.Points[0].x * 0.0001f);
		canvas->ReleaseCanvas(cx);
		return true;
	});
}

float Eval(float x, float y)
{
	//return 0.0005f * (x - sqrt(y));
	return 0.0005f * (y - x*x);
	//return 0.0005f * (x * x + y * 10.0f);
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

void Render(xo::Canvas2D* canvas, int cx, int cy, float scale)
{
	//canvas->Fill(xoColor::White());
	//xoBox box(0, 0, 7, 7);
	//box.Offset(cx - box.Width() / 2.0f, cy - box.Height() / 2.0f);
	//canvas->FillRect(box, xoColor::RGBA(200, 50, 50, 255));
	double start = xo::TimeAccurateSeconds();

	uint8_t lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = i;
		//lut[i] = 2 * abs(127 - i);
		//lut[i] = xoLinear2SRGB(i / 255.0f);
	}
	const float iscale = 255.0f / scale;

	int height = canvas->Height();
	int width = canvas->Width();
	// This omp directive gives close to an 8x speedup on VS 2015, quad core skylake.
	#pragma omp parallel for
	for (int y = 0; y < height; y++)
	{
		float yf = scale * (float) (cy - y); // we invert Y, so that up is positive
		xo::RGBA* line = (xo::RGBA*) canvas->RowPtr(y);
		for (int x = 0; x < width; x++)
		{
			float xf = scale * (float) (x - cx);
		
			float v = iscale * Eval2(xf, yf);
			
			// This is useful for illustration - having a gradient either side of the zero line
			//float v = 127.0f + iscale * Eval(xf, yf));

			int ilut = xo::Clamp((int) v, 0, 255);
			uint8_t lum = lut[ilut];
			line[x] = xo::RGBA::Make(lum, lum, lum, 255);
		}
	}
	canvas->Invalidate(xo::Box(0, 0, canvas->Width(), canvas->Height()));

	xo::Trace("canvas render: %.3f ms\n", 1000.0f * (xo::TimeAccurateSeconds() - start));
}