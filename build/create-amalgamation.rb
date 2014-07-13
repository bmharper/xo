# This script builds "nuDom-amalgamation.cpp" and "nuDom-amalgamation.h"

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
nudom/nuApiDecl.h
nudom/nuBase_SystemIncludes.h
dependencies/biggle.h
dependencies/biggle_additions.h
)

parts_cpp_2 = %w(
nudom/nuBase.h
) + local_includes + %w(
nudom/nuBase_Vector.h
nudom/nuBase_Fmt.h
nudom/nuString.h
dependencies/Panacea/Strings/fmt.h
nudom/nuPlatform.h
nudom/nuTags.h
nudom/nuDefs.h
nudom/nuCloneHelpers.h
nudom/nuEvent.h
nudom/nuMem.h
nudom/nuStringTable.h
nudom/nuStyle.h
nudom/nuStyleParser.h
nudom/nuSysWnd.h
nudom/warnings.h
nudom/Dom/nuDomEl.h
nudom/Dom/nuDomNode.h
nudom/Dom/nuDomText.h
nudom/Image/nuImage.h
nudom/Image/nuImageStore.h
nudom/nuDoc.h
nudom/nuDocGroup.h
nudom/Text/nuTextDefs.h
nudom/Text/nuGlyphCache.h
nudom/Text/nuFontStore.h
nudom/Render/nuVertexTypes.h
nudom/Render/nuRenderGL_Defs.h
nudom/Render/nuRenderDX_Defs.h
nudom/Shaders/Processed_glsl/CurveShader.h
nudom/Shaders/Processed_glsl/FillShader.h
nudom/Shaders/Processed_glsl/FillTexShader.h
nudom/Shaders/Processed_glsl/RectShader.h
nudom/Shaders/Processed_glsl/TextRGBShader.h
nudom/Shaders/Processed_glsl/TextWholeShader.h
nudom/Shaders/Processed_hlsl/FillShader.h
nudom/Shaders/Processed_hlsl/RectShader.h
nudom/Shaders/Processed_hlsl/TextRGBShader.h
nudom/Shaders/Processed_hlsl/TextWholeShader.h
nudom/Render/nuRenderBase.h
nudom/Render/nuRenderDomEl.h
nudom/Render/nuRenderDoc.h
nudom/Render/nuRenderDX.h
nudom/Render/nuRenderer.h
nudom/Render/nuRenderGL.h
nudom/Render/nuRenderStack.h
nudom/Render/nuStyleResolve.h
nudom/Render/nuTextureAtlas.h
nudom/Layout/nuLayout.h
nudom/Layout/nuTextLayout.h

nudom/nuDefs.cpp
nudom/nuEvent.cpp
nudom/nuMem.cpp
nudom/nuMsgLoop_Windows.cpp
nudom/nuPlatform.cpp
nudom/nuString.cpp
nudom/nuStringTable.cpp
nudom/nuStyle.cpp
nudom/nuStyleParser.cpp
nudom/nuSysWnd.cpp
nudom/nuDocGroup.cpp
nudom/nuDocGroup_Windows.cpp
nudom/Dom/nuDomEl.cpp
nudom/Dom/nuDomNode.cpp
nudom/Dom/nuDomText.cpp
nudom/Image/nuImage.cpp
nudom/Image/nuImageStore.cpp
nudom/Layout/nuLayout.cpp
nudom/Layout/nuTextLayout.cpp
nudom/Render/nuRenderBase.cpp
nudom/Render/nuRenderDX.cpp
nudom/Render/nuRenderDX_Defs.cpp
nudom/Render/nuRenderer.cpp
nudom/Render/nuRenderGL.cpp
nudom/Render/nuRenderGL_Defs.cpp
nudom/Render/nuRenderDoc.cpp
nudom/Render/nuRenderDomEl.cpp
nudom/Render/nuRenderStack.cpp
nudom/Render/nuStyleResolve.cpp
nudom/Render/nuTextureAtlas.cpp
nudom/Render/nuVertexTypes.cpp
nudom/Text/nuFontStore.cpp
nudom/Text/nuGlyphCache.cpp
nudom/Text/nuTextDefs.cpp
nudom/nuDoc.cpp
nudom/Shaders/Processed_glsl/CurveShader.cpp
nudom/Shaders/Processed_glsl/FillShader.cpp
nudom/Shaders/Processed_glsl/FillTexShader.cpp
nudom/Shaders/Processed_glsl/RectShader.cpp
nudom/Shaders/Processed_glsl/TextRGBShader.cpp
nudom/Shaders/Processed_glsl/TextWholeShader.cpp
nudom/Shaders/Processed_hlsl/FillShader.cpp
nudom/Shaders/Processed_hlsl/RectShader.cpp
nudom/Shaders/Processed_hlsl/TextRGBShader.cpp
nudom/Shaders/Processed_hlsl/TextWholeShader.cpp
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
nudom/nuApiDecl.h
nudom/nuBase_SystemIncludes.h
)

parts_h_2 = %w(
nudom/nuBase.h
) + local_includes + %w(
nudom/nuBase_Vector.h
nudom/nuBase_Fmt.h
nudom/nuString.h
dependencies/Panacea/Strings/fmt.h
nudom/nuPlatform.h
nudom/nuTags.h
nudom/nuDefs.h
nudom/nuMem.h
nudom/nuCloneHelpers.h
nudom/nuEvent.h
nudom/nuStyle.h
nudom/Dom/nuDomEl.h
nudom/Dom/nuDomNode.h
nudom/Dom/nuDomText.h
nudom/nuStringTable.h
nudom/Image/nuImage.h
nudom/Image/nuImageStore.h
nudom/nuDoc.h
nudom/nuDocGroup.h
nudom/nuDom.h
nudom/nuSysWnd.h
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

#define NU_BUILD_OPENGL 1
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
File.open("amalgamation/nuDom-amalgamation.cpp", "wb") { |f| f.write(amal_cpp) }
File.open("amalgamation/nuDom-amalgamation.h", "wb") { |f| f.write(amal_h) }
