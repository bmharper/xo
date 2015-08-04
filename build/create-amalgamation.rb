# This script builds the "amalgamation" files

NewLine = "\r\n"

local_includes = %w(
dependencies/agg/include/agg_basics.h
dependencies/agg/include/agg_math.h
dependencies/agg/include/agg_array.h
dependencies/agg/include/agg_curves.h
dependencies/agg/include/agg_gamma_functions.h
dependencies/agg/include/agg_gamma_lut.h
dependencies/agg/include/agg_color_rgba.h
dependencies/agg/include/agg_vertex_sequence.h
dependencies/agg/include/agg_shorten_path.h
dependencies/agg/include/agg_math_stroke.h
dependencies/agg/include/agg_vcgen_stroke.h
dependencies/agg/include/agg_conv_adaptor_vpgen.h
dependencies/agg/include/agg_conv_adaptor_vcgen.h
dependencies/agg/include/agg_conv_stroke.h
dependencies/agg/include/agg_conv_curve.h
dependencies/agg/include/agg_vpgen_clip_polyline.h
dependencies/agg/include/agg_vpgen_clip_polygon.h
dependencies/agg/include/agg_conv_clip_polyline.h
dependencies/agg/include/agg_conv_clip_polygon.h
dependencies/agg/include/agg_bezier_arc.h
dependencies/agg/include/agg_path_storage.h
dependencies/agg/include/agg_rendering_buffer.h
dependencies/agg/include/agg_pixfmt_base.h
dependencies/agg/include/agg_pixfmt_rgba.h
dependencies/agg/include/agg_clip_liang_barsky.h
dependencies/agg/include/agg_rasterizer_cells_aa.h
dependencies/agg/include/agg_rasterizer_sl_clip.h
dependencies/agg/include/agg_rasterizer_scanline_aa_nogamma.h
dependencies/agg/include/agg_rasterizer_scanline_aa.h
dependencies/agg/include/agg_renderer_base.h
dependencies/agg/include/agg_renderer_scanline.h
dependencies/agg/include/agg_scanline_u.h
dependencies/Panacea/Platform/coredefs.h
dependencies/Panacea/Platform/compiler.h
dependencies/Panacea/Platform/stdint.h
dependencies/Panacea/Platform/err.h
dependencies/Panacea/Containers/cont_utils.h
dependencies/Panacea/Containers/pvect.h
dependencies/Panacea/Containers/podvec.h
dependencies/Panacea/Platform/cpu.h
dependencies/Panacea/Platform/process.h
dependencies/Panacea/Platform/thread.h
dependencies/Panacea/Platform/syncprims.h
dependencies/Panacea/Platform/timeprims.h
dependencies/Panacea/Platform/filesystem.h
dependencies/Panacea/Platform/ConvertUTF.h
dependencies/Panacea/Containers/queue.h
dependencies/Panacea/Other/aligned_malloc.h
dependencies/Panacea/Other/StackAllocators.h
dependencies/Panacea/Bits/BitMap.h
dependencies/Panacea/fhash/fhashtable.h
dependencies/Panacea/Vec/VecPrim.h
dependencies/Panacea/Vec/VecDef.h
dependencies/Panacea/Vec/Vec2.h
dependencies/Panacea/Vec/Vec3.h
dependencies/Panacea/Vec/Vec4.h
dependencies/Panacea/Vec/Mat4.h
dependencies/Panacea/Vec/VecUndef.h
dependencies/hash/xxhash.h
)

parts_cpp_1 = %w(
xo/xoPlatformDefine.h
xo/xoBase_SystemIncludes.h
dependencies/GL/gl_xo.h
dependencies/GL/wgl_xo.h
)

parts_cpp_2 = %w(
xo/xoBase.h
) + local_includes + %w(
xo/xoBase_Vector.h
xo/xoBase_Fmt.h
xo/xoString.h
dependencies/Panacea/Strings/fmt.h
xo/xoPlatform.h
xo/xoTags.h
xo/xoDefs.h
xo/xoCloneHelpers.h
xo/xoEvent.h
xo/xoMem.h
xo/xoStringTable.h
xo/xoStyle.h
xo/xoSysWnd.h
xo/warnings.h
xo/Dom/xoDomEl.h
xo/Dom/xoDomNode.h
xo/Dom/xoDomText.h
xo/Dom/xoDomCanvas.h
xo/Image/xoImage.h
xo/Image/xoImageStore.h
xo/Canvas/xoCanvas2D.h
xo/xoDocUI.h
xo/xoDoc.h
xo/xoDocGroup.h
xo/Text/xoTextDefs.h
xo/Text/xoGlyphCache.h
xo/Text/xoFontStore.h
xo/Render/xoVertexTypes.h
xo/Render/xoRenderGL_Defs.h
xo/Render/xoRenderDX_Defs.h
xo/Shaders/Processed_glsl/CurveShader.h
xo/Shaders/Processed_glsl/FillShader.h
xo/Shaders/Processed_glsl/FillTexShader.h
xo/Shaders/Processed_glsl/RectShader.h
xo/Shaders/Processed_glsl/Rect2Shader.h
xo/Shaders/Processed_glsl/TextRGBShader.h
xo/Shaders/Processed_glsl/TextWholeShader.h
xo/Shaders/Processed_hlsl/FillShader.h
xo/Shaders/Processed_hlsl/FillTexShader.h
xo/Shaders/Processed_hlsl/RectShader.h
xo/Shaders/Processed_hlsl/TextRGBShader.h
xo/Shaders/Processed_hlsl/TextWholeShader.h
xo/Render/xoRenderBase.h
xo/Render/xoRenderDomEl.h
xo/Render/xoRenderDoc.h
xo/Render/xoRenderDX.h
xo/Render/xoRenderer.h
xo/Render/xoRenderGL.h
xo/Render/xoRenderStack.h
xo/Render/xoStyleResolve.h
xo/Render/xoTextureAtlas.h
xo/Layout/xoBoxLayout3.h
xo/Layout/xoLayout.h
xo/Layout/xoLayout2.h
xo/Layout/xoLayout3.h
xo/Layout/xoTextLayout.h
xo/Parse/xoDocParser.h

xo/xoDefs.cpp
xo/xoEvent.cpp
xo/xoMem.cpp
xo/xoMsgLoop.cpp
xo/xoPlatform.cpp
xo/xoTags.cpp
xo/xoString.cpp
xo/xoStringTable.cpp
xo/xoStyle.cpp
xo/xoSysWnd.cpp
xo/xoDocGroup.cpp
xo/xoDocGroup_Windows.cpp
xo/Dom/xoDomEl.cpp
xo/Dom/xoDomNode.cpp
xo/Dom/xoDomText.cpp
xo/Dom/xoDomCanvas.cpp
xo/Image/xoImage.cpp
xo/Image/xoImageStore.cpp
xo/Canvas/xoCanvas2D.cpp
xo/Layout/xoBoxLayout3.cpp
xo/Layout/xoLayout.cpp
xo/Layout/xoLayout2.cpp
xo/Layout/xoLayout3.cpp
xo/Layout/xoTextLayout.cpp
xo/Parse/xoDocParser.cpp
xo/Render/xoRenderBase.cpp
xo/Render/xoRenderDX.cpp
xo/Render/xoRenderDX_Defs.cpp
xo/Render/xoRenderer.cpp
xo/Render/xoRenderGL.cpp
xo/Render/xoRenderGL_Defs.cpp
xo/Render/xoRenderDoc.cpp
xo/Render/xoRenderDomEl.cpp
xo/Render/xoRenderStack.cpp
xo/Render/xoStyleResolve.cpp
xo/Render/xoTextureAtlas.cpp
xo/Render/xoVertexTypes.cpp
xo/Text/xoFontStore.cpp
xo/Text/xoGlyphCache.cpp
xo/Text/xoTextDefs.cpp
xo/xoDocUI.cpp
xo/xoDoc.cpp
xo/Shaders/Processed_glsl/CurveShader.cpp
xo/Shaders/Processed_glsl/FillShader.cpp
xo/Shaders/Processed_glsl/FillTexShader.cpp
xo/Shaders/Processed_glsl/RectShader.cpp
xo/Shaders/Processed_glsl/Rect2Shader.cpp
xo/Shaders/Processed_glsl/TextRGBShader.cpp
xo/Shaders/Processed_glsl/TextWholeShader.cpp
xo/Shaders/Processed_hlsl/FillShader.cpp
xo/Shaders/Processed_hlsl/FillTexShader.cpp
xo/Shaders/Processed_hlsl/RectShader.cpp
xo/Shaders/Processed_hlsl/TextRGBShader.cpp
xo/Shaders/Processed_hlsl/TextWholeShader.cpp
dependencies/Panacea/Containers/queue.cpp
dependencies/Panacea/Platform/cpu.cpp
dependencies/Panacea/Platform/err.cpp
dependencies/Panacea/Platform/process.cpp
dependencies/Panacea/Platform/filesystem.cpp
dependencies/Panacea/Platform/syncprims.cpp
dependencies/Panacea/Platform/timeprims.cpp
dependencies/Panacea/Platform/thread.cpp
dependencies/Panacea/Platform/ConvertUTF.cpp
dependencies/Panacea/Strings/fmt.cpp
dependencies/stb_image.c
dependencies/hash/xxhash.cpp
dependencies/GL/gl_xo.cpp
dependencies/agg/src/agg_vcgen_stroke.cpp
dependencies/agg/src/agg_vpgen_clip_polygon.cpp
dependencies/agg/src/agg_vpgen_clip_polyline.cpp
)

# Omit the first section from the platform-specific GL files. This first section is a bunch of static
# functions that are common to all files generated by the gl-loader-generator. You will see that
# gl_xo.cpp is included in its entirety, so that is the one final home of these functions.
parts_gl_platform_cpp = [
	{:filename => "dependencies/GL/wgl_xo.cpp", :platform => :windesktop, :start => 98},
	{:filename => "dependencies/GL/glx_xo.cpp", :platform => :linux, :start => 98},
]

parts_h_1 = %w(
xo/xoPlatformDefine.h
xo/xoBase_SystemIncludes.h
)

parts_h_2 = %w(
xo/xoBase.h
) + local_includes + %w(
xo/xoBase_Vector.h
xo/xoBase_Fmt.h
xo/xoString.h
dependencies/Panacea/Strings/fmt.h
xo/xoPlatform.h
xo/xoTags.h
xo/xoDefs.h
xo/xoMem.h
xo/xoCloneHelpers.h
xo/xoEvent.h
xo/xoStyle.h
xo/Dom/xoDomEl.h
xo/Dom/xoDomNode.h
xo/Dom/xoDomText.h
xo/Dom/xoDomCanvas.h
xo/xoStringTable.h
xo/Image/xoImage.h
xo/Image/xoImageStore.h
xo/Canvas/xoCanvas2D.h
xo/xoDocUI.h
xo/xoDoc.h
xo/xoDocGroup.h
xo/xo.h
xo/xoSysWnd.h
)

prelude_common = <<END
// These warnings are for stb_image.c
#ifdef _MSC_VER
#pragma warning(push)          // push XO-AMALGAMATION-DISABLED-WARNINGS
#pragma warning(disable: 4251) // class needs to have dll-interface to be used by clients of class
#pragma warning(disable: 6001) // using uninitialized memory
#pragma warning(disable: 6246) // local declaration hides name in outer scope
#pragma warning(disable: 6262) // stack size
#pragma warning(disable: 6385) // reading invalid data
#endif

#ifndef ASSERT
	#define TEMP_ASSERT
	#ifdef _DEBUG
		#define ASSERT(condition) (void)0
	#else
		#define ASSERT(condition) assert(condition)
	#endif
#endif

// Keep this in sync with xo/pch.h
#define PROJECT_XO 1

// This must be valid before xoPlatformDefine.h is included, so we can't use XO_PLATFORM_WIN_DESKTOP
#if defined(_WIN32) && !defined(XO_EXCLUDE_DIRECTX)
	#define XO_BUILD_DIRECTX 1
#else
	#define XO_BUILD_DIRECTX 0
#endif

#if !defined(XO_EXCLUDE_OPENGL)
	#define XO_BUILD_OPENGL 1
#endif
END

epilogue_common = <<END
#ifdef TEMP_ASSERT
	#undef TEMP_ASSERT
	#undef ASSERT
#endif

#ifdef _MSC_VER
#pragma warning(pop) // pop of XO-AMALGAMATION-DISABLED-WARNINGS
#endif
END

prelude_cpp = <<END
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <io.h>
	#include <sys/locking.h>
	#include <fcntl.h>
	#include <Psapi.h>
#else
	#include <unistd.h>
	#include <fcntl.h>
#endif

#include <algorithm>	// std::min/max
#include <vector>
#include <sys/stat.h>

//#include "xo-amalgamation-freetype.h"
END

prelude_h = <<END
#ifndef XO_AMALGAMATION_H_INCLUDED
#define XO_AMALGAMATION_H_INCLUDED
END

epilogue_h = <<END
#endif // XO_AMALGAMATION_H_INCLUDED
END

##################################################################################################
# Functions
##################################################################################################

def fixlines(str)
	r = ""
	str.each_line { |line| r << line.rstrip + NewLine }
	return r
end

def process_range(filename, start_keep_line, end_keep_line, keep_includes)
	iline = -1
	result = ""
	input = File.open(filename, "rb") { |f| f.read }
	input.each_line { |line|
		iline += 1
		next if start_keep_line != nil && iline < start_keep_line
		break if end_keep_line != nil && iline > end_keep_line
		result += line
	}
	return process_string(result, keep_includes)
end

def read_file(filename)
	return File.open(filename, "rb") { |f| f.read }
end

def process_file(filename, keep_includes)
	return process_string(read_file(filename), keep_includes)
end

def process_string(text, keep_includes)
	result = ""
	text.each_line { |line|
		next if !keep_includes && line =~ /#include/
		next if line =~ /#pragma once/
		line.rstrip!
		line.chop! if line[-1] == "\x1A"
		result += line + NewLine
	}
	return result
end

def concat_proc(files)
	all = ""
	files.each { |item|
		all << yield(item)
		all << NewLine
	}
	return all
end

def concat(files, keep_includes)
	return concat_proc(files) { |file|
		process_file(file, keep_includes)
	}
end

def platform_ifdef(platform)
	case platform
	when :linux then		return '#ifdef XO_PLATFORM_LINUX_DESKTOP'
	when :windesktop then	return '#ifdef XO_PLATFORM_WIN_DESKTOP'
	else					raise "Unknown platform #{platform}"
	end
end

def concat_gl_platform(parts)
	# These static symbols are used across all the GL loader files, so we make them unique
	make_unique = %w(
	g_extensionMapSize
	ExtensionMap
	FindExtEntry
	LoadExtByName
	ClearExtensionVars
	)

	return concat_proc(parts) { |item|
		top = platform_ifdef(item[:platform]) + NewLine
		bottom = '#endif' + NewLine
		main = process_range(item[:filename], item[:start], item[:end], false)
		make_unique.each { |sym|
			sym = sym.strip
			main.gsub!(sym, sym + "_" + item[:platform].to_s)
		}
		return top + main + bottom
	}
end

`ruby build/preprocess-freetype.rb`

prelude_cpp += NewLine + read_file("amalgamation/xo-amalgamation-freetype.h") + NewLine
File::delete("amalgamation/xo-amalgamation-freetype.h")

amal_cpp = prelude_cpp + prelude_common + concat(parts_cpp_1, true) + concat(parts_cpp_2, false) + concat_gl_platform(parts_gl_platform_cpp) + epilogue_common
amal_h = prelude_h + prelude_common + concat(parts_h_1, true) + concat(parts_h_2, false) + epilogue_common + epilogue_h
amal_cpp = fixlines(amal_cpp)
amal_h = fixlines(amal_h)
Dir.mkdir("amalgamation") if !Dir.exist?("amalgamation")
File.open("amalgamation/xo-amalgamation.cpp", "wb") { |f| f.write(amal_cpp) }
File.open("amalgamation/xo-amalgamation.h", "wb") { |f| f.write(amal_h) }
