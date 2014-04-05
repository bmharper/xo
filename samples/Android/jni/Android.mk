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

NU_SRC = \
../../../templates/nuAndroid.cpp \
../../../nudom/nuDefs.cpp \
../../../nudom/nuDoc.cpp \
../../../nudom/nuDomEl.cpp \
../../../nudom/nuEvent.cpp \
../../../nudom/nuLayout.cpp \
../../../nudom/nuMem.cpp \
../../../nudom/nuPlatform.cpp \
../../../nudom/nuString.cpp \
../../../nudom/nuStringTable.cpp \
../../../nudom/nuStyle.cpp \
../../../nudom/nuStyleParser.cpp \
../../../nudom/nuSysWnd.cpp \
../../../nudom/nuDocGroup.cpp \
../../../nudom/Image/nuImage.cpp \
../../../nudom/Image/nuImageStore.cpp \
../../../nudom/Render/nuRenderer.cpp \
../../../nudom/Render/nuRenderBase.cpp \
../../../nudom/Render/nuRenderGL.cpp \
../../../nudom/Render/nuRenderGL_Defs.cpp \
../../../nudom/Render/nuRenderDoc.cpp \
../../../nudom/Render/nuRenderDomEl.cpp \
../../../nudom/Render/nuRenderStack.cpp \
../../../nudom/Render/nuStyleResolve.cpp \
../../../nudom/Render/nuTextureAtlas.cpp \
../../../nudom/Text/nuFontStore.cpp \
../../../nudom/Text/nuGlyphCache.cpp \
../../../nudom/Text/nuTextDefs.cpp \
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

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../nudom $(LOCAL_PATH)/../../../dependencies/freetype/include
LOCAL_MODULE     := libnudom
LOCAL_CFLAGS     := -Werror -DFT2_BUILD_LIBRARY
LOCAL_CPPFLAGS   := -Werror -std=c++11
LOCAL_SRC_FILES  := $(FREETYPE_SRC) $(NU_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
