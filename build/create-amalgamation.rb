# This script builds "xo-amalgamation.cpp" and "xo-amalgamation.h"

# STATUS:
# I stopped working on this once I started getting Freetype compilation errors.
# I have not made any attempt to incorporate Freetype into the amalgamation, but I believe
# it is necessary to do so in order to have the compilation experience that the amalgamation
# strives for.

##################################################################################################
# Constants
##################################################################################################

NewLine = "\r\n"

local_includes = %w(
dependencies/Panacea/Platform/coredefs.h
dependencies/Panacea/Platform/compiler.h
dependencies/Panacea/Platform/stdint.h
dependencies/Panacea/Platform/err.h
dependencies/Panacea/Containers/cont_utils.h
dependencies/Panacea/Containers/pvect.h
dependencies/Panacea/Containers/podvec.h
dependencies/Panacea/Platform/cpu.h
dependencies/Panacea/Platform/process.h
dependencies/Panacea/Platform/syncprims.h
dependencies/Panacea/Platform/timeprims.h
dependencies/Panacea/Platform/thread.h
dependencies/Panacea/Containers/queue.h
dependencies/Panacea/Other/aligned_malloc.h
dependencies/Panacea/Other/StackAllocators.h
dependencies/Panacea/Bits/BitMap.h
dependencies/Panacea/fhash/fhashtable.h
dependencies/Panacea/Vec/VecPrim.h
dependencies/Panacea/Vec/Vec2.h
dependencies/Panacea/Vec/Vec3.h
dependencies/Panacea/Vec/Vec4.h
dependencies/Panacea/Vec/Mat4.h
)

parts_cpp_1 = %w(
xo/xoApiDecl.h
xo/xoBase_SystemIncludes.h
dependencies/biggle.h
dependencies/biggle_additions.h
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
xo/xoStyleParser.h
xo/xoSysWnd.h
xo/warnings.h
xo/Dom/xoDomEl.h
xo/Dom/xoDomNode.h
xo/Dom/xoDomText.h
xo/Image/xoImage.h
xo/Image/xoImageStore.h
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
xo/Shaders/Processed_glsl/TextRGBShader.h
xo/Shaders/Processed_glsl/TextWholeShader.h
xo/Shaders/Processed_hlsl/FillShader.h
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
xo/Layout/xoLayout.h
xo/Layout/xoTextLayout.h

xo/xoDefs.cpp
xo/xoEvent.cpp
xo/xoMem.cpp
xo/xoMsgLoop_Windows.cpp
xo/xoPlatform.cpp
xo/xoString.cpp
xo/xoStringTable.cpp
xo/xoStyle.cpp
xo/xoStyleParser.cpp
xo/xoSysWnd.cpp
xo/xoDocGroup.cpp
xo/xoDocGroup_Windows.cpp
xo/Dom/xoDomEl.cpp
xo/Dom/xoDomNode.cpp
xo/Dom/xoDomText.cpp
xo/Image/xoImage.cpp
xo/Image/xoImageStore.cpp
xo/Layout/xoLayout.cpp
xo/Layout/xoTextLayout.cpp
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
xo/xoDoc.cpp
xo/Shaders/Processed_glsl/CurveShader.cpp
xo/Shaders/Processed_glsl/FillShader.cpp
xo/Shaders/Processed_glsl/FillTexShader.cpp
xo/Shaders/Processed_glsl/RectShader.cpp
xo/Shaders/Processed_glsl/TextRGBShader.cpp
xo/Shaders/Processed_glsl/TextWholeShader.cpp
xo/Shaders/Processed_hlsl/FillShader.cpp
xo/Shaders/Processed_hlsl/RectShader.cpp
xo/Shaders/Processed_hlsl/TextRGBShader.cpp
xo/Shaders/Processed_hlsl/TextWholeShader.cpp
dependencies/Panacea/Containers/queue.cpp
dependencies/Panacea/Platform/cpu.cpp
dependencies/Panacea/Platform/err.cpp
dependencies/Panacea/Platform/process.cpp
dependencies/Panacea/Platform/syncprims.cpp
dependencies/Panacea/Platform/thread.cpp
dependencies/Panacea/Strings/ConvertUTF.cpp
dependencies/Panacea/Strings/fmt.cpp
dependencies/glext.cpp
dependencies/stb_image.c
)

parts_h_1 = %w(
xo/xoApiDecl.h
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
xo/xoStringTable.h
xo/Image/xoImage.h
xo/Image/xoImageStore.h
xo/xoDoc.h
xo/xoDocGroup.h
xo/xo.h
xo/xoSysWnd.h
)

prelude = <<END
// These warnings are for stb_image.c
#ifdef _MSC_VER
#pragma warning(push)
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

#define XO_BUILD_OPENGL 1
END

epilogue = <<END
#ifdef TEMP_ASSERT
	#undef TEMP_ASSERT
	#undef ASSERT
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
END

prelude_cpp = <<END
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS
END

##################################################################################################
# Functions
##################################################################################################

def fixlines(str)
	r = ""
	str.each_line { |line| r << line.rstrip + NewLine }
	return r
end

def process(filename, keep_includes)
	result = ""
	input = File.open(filename, "rb") { |f| f.read }
	input.each_line { |line|
		next if !keep_includes && line =~ /#include/
		next if line =~ /#pragma once/
		result += line
	}
	return result
end

def concat(files, keep_includes)
	all = ""
	files.each { |name|
		all << process(name, keep_includes)
		all << NewLine
	}
	return all
end

amal_cpp = prelude_cpp + prelude + concat(parts_cpp_1, true) + concat(parts_cpp_2, false) + epilogue
amal_h = prelude + concat(parts_h_1, true) + concat(parts_h_2, false) + epilogue
amal_cpp = fixlines(amal_cpp)
amal_h = fixlines(amal_h)
#amal_cpp.gsub!(/[(^\r)]\n/, "\1\r\n")
#amal_h.gsub!(/[(^\r)]\n/, "\1\r\n")
Dir.mkdir("amalgamation") if !Dir.exist?("amalgamation")
File.open("amalgamation/xo-amalgamation.cpp", "wb") { |f| f.write(amal_cpp) }
File.open("amalgamation/xo-amalgamation.h", "wb") { |f| f.write(amal_h) }
