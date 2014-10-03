#pragma once

#ifndef ASSERT
	#define TEMP_ASSERT
	#ifdef _DEBUG
		#define ASSERT(condition) (void)0
	#else
		#define ASSERT(condition) assert(condition)
	#endif
#endif

#include "../dependencies/agg/include/agg_arrowhead.h"
#include "../dependencies/agg/include/agg_basics.h"
#include "../dependencies/agg/include/agg_clip_liang_barsky.h"
#include "../dependencies/agg/include/agg_conv_stroke.h"
#include "../dependencies/agg/include/agg_conv_dash.h"
#include "../dependencies/agg/include/agg_conv_curve.h"
#include "../dependencies/agg/include/agg_conv_contour.h"
#include "../dependencies/agg/include/agg_conv_clip_polyline.h"
#include "../dependencies/agg/include/agg_conv_clip_polygon.h"
#include "../dependencies/agg/include/agg_conv_smooth_poly1.h"
#include "../dependencies/agg/include/agg_conv_transform.h"
#include "../dependencies/agg/include/agg_conv_marker.h"
#include "../dependencies/agg/include/agg_ellipse.h"
#include "../dependencies/agg/include/agg_image_accessors.h"
#include "../dependencies/agg/include/agg_path_storage.h"
#include "../dependencies/agg/include/agg_path_storage_integer.h"
#include "../dependencies/agg/include/agg_pixfmt_gray.h"
#include "../dependencies/agg/include/agg_pixfmt_rgb.h"
#include "../dependencies/agg/include/agg_pixfmt_rgba.h"
#include "../dependencies/agg/include/agg_rasterizer_outline.h"
#include "../dependencies/agg/include/agg_rasterizer_outline_aa.h"
#include "../dependencies/agg/include/agg_rasterizer_scanline_aa.h"
#include "../dependencies/agg/include/agg_renderer_scanline.h"
#include "../dependencies/agg/include/agg_renderer_primitives.h"
#include "../dependencies/agg/include/agg_renderer_outline_aa.h"
#include "../dependencies/agg/include/agg_rendering_buffer.h"
#include "../dependencies/agg/include/agg_scanline_u.h"
#include "../dependencies/agg/include/agg_scanline_p.h"
#include "../dependencies/agg/include/agg_span_image_filter_gray.h"
#include "../dependencies/agg/include/agg_span_image_filter_rgb.h"
#include "../dependencies/agg/include/agg_span_image_filter_rgba.h"
#include "../dependencies/agg/include/agg_span_allocator.h"
#include "../dependencies/agg/include/agg_span_pattern_gray.h"
#include "../dependencies/agg/include/agg_span_pattern_rgba.h"
#include "../dependencies/agg/include/agg_trans_affine.h"
#include "../dependencies/agg/include/agg_vcgen_markers_term.h"
#include "../dependencies/agg/include/agg_blur.h"

#include "../dependencies/Panacea/Platform/coredefs.h"
#include "../dependencies/Panacea/Platform/stdint.h"
#include "../dependencies/Panacea/Containers/pvect.h"
#include "../dependencies/Panacea/Containers/podvec.h"
#include "../dependencies/Panacea/Containers/queue.h"
#include "../dependencies/Panacea/Platform/cpu.h"
#include "../dependencies/Panacea/Platform/err.h"
#include "../dependencies/Panacea/Platform/filesystem.h"
#include "../dependencies/Panacea/Platform/process.h"
#include "../dependencies/Panacea/Platform/syncprims.h"
#include "../dependencies/Panacea/Platform/timeprims.h"
#include "../dependencies/Panacea/Platform/thread.h"
#include "../dependencies/Panacea/Other/StackAllocators.h"
#include "../dependencies/Panacea/Bits/BitMap.h"
#include "../dependencies/Panacea/fhash/fhashtable.h"
#include "../dependencies/Panacea/Strings/ConvertUTF.h"
#include "../dependencies/Panacea/Vec/Vec2.h"
#include "../dependencies/Panacea/Vec/Vec3.h"
#include "../dependencies/Panacea/Vec/Vec4.h"
#include "../dependencies/Panacea/Vec/Mat4.h"

#include "../dependencies/hash/xxhash.h"

#ifdef TEMP_ASSERT
	#undef TEMP_ASSERT
	#undef ASSERT
#endif
