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
../../../nudom/xoDefs.cpp \
../../../nudom/xoDoc.cpp \
../../../nudom/xoEvent.cpp \
../../../nudom/xoMem.cpp \
../../../nudom/xoPlatform.cpp \
../../../nudom/xoString.cpp \
../../../nudom/xoStringTable.cpp \
../../../nudom/xoStyle.cpp \
../../../nudom/xoStyleParser.cpp \
../../../nudom/xoSysWnd.cpp \
../../../nudom/xoDocGroup.cpp \
../../../nudom/Dom/xoDomEl.cpp \
../../../nudom/Dom/xoDomNode.cpp \
../../../nudom/Dom/xoDomText.cpp \
../../../nudom/Image/xoImage.cpp \
../../../nudom/Image/xoImageStore.cpp \
../../../nudom/Layout/xoLayout.cpp \
../../../nudom/Layout/xoTextLayout.cpp \
../../../nudom/Render/xoRenderer.cpp \
../../../nudom/Render/xoRenderBase.cpp \
../../../nudom/Render/xoRenderGL.cpp \
../../../nudom/Render/xoRenderGL_Defs.cpp \
../../../nudom/Render/xoRenderDoc.cpp \
../../../nudom/Render/xoRenderDomEl.cpp \
../../../nudom/Render/xoRenderStack.cpp \
../../../nudom/Render/xoStyleResolve.cpp \
../../../nudom/Render/xoTextureAtlas.cpp \
../../../nudom/Render/xoVertexTypes.cpp \
../../../nudom/Text/xoFontStore.cpp \
../../../nudom/Text/xoGlyphCache.cpp \
../../../nudom/Text/xoTextDefs.cpp \
../../../nudom/Shaders/Processed_glsl/CurveShader.cpp \
../../../nudom/Shaders/Processed_glsl/FillShader.cpp \
../../../nudom/Shaders/Processed_glsl/FillTexShader.cpp \
../../../nudom/Shaders/Processed_glsl/RectShader.cpp \
../../../nudom/Shaders/Processed_glsl/TextRGBShader.cpp \
../../../nudom/Shaders/Processed_glsl/TextWholeShader.cpp \
../../../dependencies/Panacea/Containers/queue.cpp \
../../../dependencies/Panacea/Platform/cpu.cpp \
../../../dependencies/Panacea/Platform/err.cpp \
../../../dependencies/Panacea/Platform/syncprims.cpp \
../../../dependencies/Panacea/Platform/thread.cpp \
../../../dependencies/Panacea/Strings/fmt.cpp \

MY_SRC = ../../HelloWorld/HelloWorld.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../xo $(LOCAL_PATH)/../../../dependencies/freetype/include
LOCAL_MODULE     := libxo
LOCAL_CFLAGS     := -Werror -DFT2_BUILD_LIBRARY
LOCAL_CPPFLAGS   := -Werror -std=c++11
LOCAL_SRC_FILES  := $(FREETYPE_SRC) $(XO_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
