#include "pch.h"
#include "Canvas2D.h"
#include "../Image/Image.h"

namespace xo {

Canvas2D::Canvas2D(xo::Texture* backingImage)
    : Image(backingImage) {
	if (Image != nullptr && Image->Format == TexFormatRGBA8) {
		RenderBuff.attach((uint8_t*) Image->Data, Image->Width, Image->Height, Image->Stride);
		PixFormatRGBA_Pre.attach(RenderBuff);
		RenderBaseRGBA_Pre.attach(PixFormatRGBA_Pre);
		RenderAA_RGBA_Pre.attach(RenderBaseRGBA_Pre);
		IsAlive = Image->Data != nullptr && Image->Width != 0 && Image->Height != 0;
		InvalidRect.SetInverted();
	} else {
		IsAlive = false;
	}
}

Canvas2D::~Canvas2D() {
}

void Canvas2D::Fill(Color color) {
	if (!IsAlive)
		return;

	FillRect(Box(0, 0, Width(), Height()), color);
}

void Canvas2D::FillRect(Box box, Color color) {
	if (!IsAlive)
		return;

	RenderBaseRGBA_Pre.copy_bar(box.Left, box.Top, box.Right, box.Bottom, ColorToAgg8(color));
	InvalidRect.ExpandToFit(box);
}

void Canvas2D::StrokeRect(Box box, Color color, float linewidth) {
	if (!IsAlive)
		return;

	float v[8] = {
	    (float) box.Left, (float) box.Top,
	    (float) box.Right, (float) box.Top,
	    (float) box.Right, (float) box.Bottom,
	    (float) box.Left, (float) box.Bottom,
	};

	StrokeLine(true, 4, v, 2 * sizeof(float), color, linewidth);
}

void Canvas2D::StrokeLine(bool closed, int nvx, const float* vx, int vx_stride_bytes, Color color, float linewidth) {
	if (!IsAlive)
		return;

	RasAA.reset();
	RasAA.filling_rule(agg::fill_non_zero);

	agg::path_storage path;

	path.start_new_path();

	// emit first vertex
	path.move_to(vx[0], vx[1]);
	(char*&) vx += vx_stride_bytes;
	nvx--;

	// emit remaining vertices
	for (int i = 0; i < nvx; i++, (char *&) vx += vx_stride_bytes)
		path.line_to(vx[0], vx[1]);

	if (closed)
		path.close_polygon();

	TLineClipper                   clipped_line(path);
	TFillClipper                   clipped_fill(path);
	agg::conv_stroke<TLineClipper> clipped_line_stroked(clipped_line);
	agg::conv_stroke<TFillClipper> clipped_fill_stroked(clipped_fill);

	if (closed) {
		clipped_fill.clip_box(-linewidth, -linewidth, Width() + linewidth, Height() + linewidth);
		clipped_fill_stroked.line_cap(agg::butt_cap);
		clipped_fill_stroked.line_join(agg::miter_join);
		clipped_fill_stroked.width(linewidth);
		RasAA.add_path(clipped_fill_stroked);
	} else {
		clipped_line.clip_box(-linewidth, -linewidth, Width() + linewidth, Height() + linewidth);
		clipped_line_stroked.line_cap(agg::butt_cap);
		clipped_line_stroked.line_join(agg::miter_join);
		clipped_line_stroked.width(linewidth);
		RasAA.add_path(clipped_line_stroked);
	}

	RenderAA_RGBA_Pre.color(ColorToAgg8(color));

	RenderScanlines();
}

void Canvas2D::StrokeLine(float x1, float y1, float x2, float y2, Color color, float linewidth) {
	float vx[4] = {
		x1,y1,
		x2,y2,
	};
	StrokeLine(false, 2, vx, 2 * sizeof(float), color, linewidth);
}

void Canvas2D::StrokeCircle(float x, float y, float radius, Color color, float linewidth) {
	if (!IsAlive)
		return;

	RasAA.reset();
	agg::path_storage path;
	path.start_new_path();
	agg::ellipse elps;
	elps.init(x, y, radius, radius);
	path.concat_path(elps, 0);

	TLineClipper clipped_line(path);
	clipped_line.clip_box(-linewidth, -linewidth, Width() + linewidth, Height() + linewidth);
	agg::conv_stroke<TLineClipper> clipped_line_stroked(clipped_line);
	clipped_line_stroked.width(linewidth);
	RasAA.add_path(clipped_line_stroked);
	RenderAA_RGBA_Pre.color(ColorToAgg8(color));
	RenderScanlines();
}

void Canvas2D::FillCircle(float x, float y, float radius, Color color) {
	if (!IsAlive)
		return;

	RasAA.reset();
	agg::path_storage path;
	path.start_new_path();
	agg::ellipse elps;
	elps.init(x, y, radius, radius);
	path.concat_path(elps, 0);

	RasAA.add_path(path);
	RenderAA_RGBA_Pre.color(ColorToAgg8(color));
	RenderScanlines();
}

void Canvas2D::RenderSVG(const char* svg) {
	try {
		agg::svg::path_renderer path;
		agg::svg::parser parse(path);
		parse.parse_mem(svg);

        typedef agg::pixfmt_bgra32 pixfmt;
        typedef agg::renderer_base<pixfmt> renderer_base;
        typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_solid;

		//int stride = key.Width * 4;
		//uint8_t* buf = (uint8_t*) MallocOrDie(stride * key.Height);
		//agg::rendering_buffer rbuf(buf, key.Width, key.Height, stride);

		pixfmt pixf(RenderBuff);
		renderer_base rb(pixf);
		renderer_solid ren(rb);

		rb.clear(agg::rgba(1,1,1, 0));

		agg::rasterizer_scanline_aa<> ras;
		agg::scanline_p8 sl;
		agg::trans_affine mtx;

		auto vb = parse.view_box();
		double vbWidth = vb[2] - vb[0];
		double vbHeight = vb[3] - vb[1];
		double scale = std::min(Width() / vbWidth, Height() / vbHeight);

		//ras.gamma(agg::gamma_power(1));
		//ras.gamma(agg::gamma_power(m_gamma.value()));
		//mtx *= agg::trans_affine_translation((m_min_x + m_max_x) * -0.5, (m_min_y + m_max_y) * -0.5);
		mtx *= agg::trans_affine_scaling(scale);
		//mtx *= agg::trans_affine_rotation(agg::deg2rad(m_rotate.value()));
		//mtx *= agg::trans_affine_translation((m_min_x + m_max_x) * 0.5 + m_x, (m_min_y + m_max_y) * 0.5 + m_y + 30);
        
		//m_path.expand(m_expand.value());
		//start_timer();
		path.render(ras, sl, ren, mtx, rb.clip_box(), 1.0);
		//double tm = elapsed_time();
		//unsigned vertex_count = m_path.vertex_count();

	} catch (const agg::svg::exception& ex) {
		Trace("Error rendering svg: %v", ex.msg());
	}
}

agg::rgba Canvas2D::ColorToAgg(Color c) {
	c = c.Premultiply();
	return agg::rgba(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

agg::rgba8 Canvas2D::ColorToAgg8(Color c) {
	c = c.Premultiply();
	return agg::rgba8(c.r, c.g, c.b, c.a);
}

void Canvas2D::RenderScanlines() {
	agg::render_scanlines(RasAA, Scanline, RenderAA_RGBA_Pre);
	InvalidRect.ExpandToFit(Box(RasAA.min_x(), RasAA.min_y(), RasAA.max_x(), RasAA.max_y()));
}

}
