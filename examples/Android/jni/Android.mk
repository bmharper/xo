LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

FREETYPE_SRC = \
../../../dependencies/freetype/src/autofit/autofit.c \
../../../dependencies/freetype/src/base/ftbase.c \
../../../dependencies/freetype/src/base/ftbitmap.c \
../../../dependencies/freetype/src/bdf/bdf.c \
../../../dependencies/freetype/src/cff/cff.c \
../../../dependencies/freetype/src/cache/ftcache.c \
../../../dependencies/freetype/src/base/ftgasp.c \
../../../dependencies/freetype/src/base/ftglyph.c \
../../../dependencies/freetype/src/gzip/ftgzip.c \
../../../dependencies/freetype/src/base/ftinit.c \
../../../dependencies/freetype/src/base/ftlcdfil.c \
../../../dependencies/freetype/src/lzw/ftlzw.c \
../../../dependencies/freetype/src/base/ftstroke.c \
../../../dependencies/freetype/src/base/ftsystem.c \
../../../dependencies/freetype/src/smooth/smooth.c \
../../../dependencies/freetype/src/base/ftbbox.c \
../../../dependencies/freetype/src/base/ftmm.c \
../../../dependencies/freetype/src/base/ftpfr.c \
../../../dependencies/freetype/src/base/ftsynth.c \
../../../dependencies/freetype/src/base/fttype1.c \
../../../dependencies/freetype/src/base/ftwinfnt.c \
../../../dependencies/freetype/src/pcf/pcf.c \
../../../dependencies/freetype/src/pfr/pfr.c \
../../../dependencies/freetype/src/psaux/psaux.c \
../../../dependencies/freetype/src/pshinter/pshinter.c \
../../../dependencies/freetype/src/psnames/psmodule.c \
../../../dependencies/freetype/src/raster/raster.c \
../../../dependencies/freetype/src/sfnt/sfnt.c \
../../../dependencies/freetype/src/truetype/truetype.c \
../../../dependencies/freetype/src/type1/type1.c \
../../../dependencies/freetype/src/cid/type1cid.c \
../../../dependencies/freetype/src/type42/type42.c \
../../../dependencies/freetype/src/winfonts/winfnt.c \

XO_SRC = \
../../../templates/xoAndroid.cpp \
../../../xo/xoDefs.cpp \
../../../xo/xoDoc.cpp \
../../../xo/xoDocUI.cpp \
../../../xo/xoDocGroup.cpp \
../../../xo/xoEvent.cpp \
../../../xo/xoMem.cpp \
../../../xo/xoMsgLoop.cpp \
../../../xo/xoPlatform.cpp \
../../../xo/xoString.cpp \
../../../xo/xoStringTable.cpp \
../../../xo/xoStyle.cpp \
../../../xo/xoSysWnd.cpp \
../../../xo/xoTags.cpp \
../../../xo/Canvas/xoCanvas2D.cpp \
../../../xo/Dom/xoDomEl.cpp \
../../../xo/Dom/xoDomCanvas.cpp \
../../../xo/Dom/xoDomNode.cpp \
../../../xo/Dom/xoDomText.cpp \
../../../xo/Image/xoImage.cpp \
../../../xo/Image/xoImageStore.cpp \
../../../xo/Layout/xoLayout.cpp \
../../../xo/Layout/xoLayout2.cpp \
../../../xo/Layout/xoTextLayout.cpp \
../../../xo/Parse/xoDocParser.cpp \
../../../xo/Render/xoRenderBase.cpp \
../../../xo/Render/xoRenderer.cpp \
../../../xo/Render/xoRenderGL.cpp \
../../../xo/Render/xoRenderGL_Defs.cpp \
../../../xo/Render/xoRenderDoc.cpp \
../../../xo/Render/xoRenderDomEl.cpp \
../../../xo/Render/xoRenderStack.cpp \
../../../xo/Render/xoStyleResolve.cpp \
../../../xo/Render/xoTextureAtlas.cpp \
../../../xo/Render/xoVertexTypes.cpp \
../../../xo/Text/xoFontStore.cpp \
../../../xo/Text/xoGlyphCache.cpp \
../../../xo/Text/xoTextDefs.cpp \
../../../xo/Shaders/Processed_glsl/FillShader.cpp \
../../../xo/Shaders/Processed_glsl/FillTexShader.cpp \
../../../xo/Shaders/Processed_glsl/RectShader.cpp \
../../../xo/Shaders/Processed_glsl/TextRGBShader.cpp \
../../../xo/Shaders/Processed_glsl/TextWholeShader.cpp \
		"dependencies/agg/src/agg_vcgen_stroke.cpp",
		"dependencies/agg/src/agg_vpgen_clip_polygon.cpp",
		"dependencies/agg/src/agg_vpgen_clip_polyline.cpp",
		"dependencies/hash/xxhash.cpp",
../../../dependencies/Panacea/Containers/queue.cpp \
../../../dependencies/Panacea/Platform/cpu.cpp \
../../../dependencies/Panacea/Platform/err.cpp \
		"dependencies/Panacea/Platform/filesystem.cpp",
		"dependencies/Panacea/Platform/process.cpp",
../../../dependencies/Panacea/Platform/syncprims.cpp \
		"dependencies/Panacea/Platform/timeprims.cpp",
../../../dependencies/Panacea/Platform/thread.cpp \
		"dependencies/Panacea/Strings/ConvertUTF.cpp",
../../../dependencies/Panacea/Strings/fmt.cpp \

MY_SRC = ../../HelloWorld/HelloWorld.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../xo $(LOCAL_PATH)/../../../dependencies/freetype/include
LOCAL_MODULE     := libxo
LOCAL_CFLAGS     := -Werror -DFT2_BUILD_LIBRARY
LOCAL_CPPFLAGS   := -Werror -std=c++11
LOCAL_SRC_FILES  := $(FREETYPE_SRC) $(XO_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
