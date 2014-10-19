LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# XO_ROOT_SRC is relative to the 'jni' directory
# XO_ROOT_INCUDES is relative to the root of the project

# These definitions use the xo source code in it's natural form inside its own repository.
# XO_ROOT_SRC := ../../..
# XO_ROOT_INCLUDES := ../..

# These definitions refer to a copy of the xo source code inside the 'jni' directory.
# Run copy-src-to-jni.bat to copy the sources in there.
XO_ROOT_SRC := xo
XO_ROOT_INCLUDES := jni/xo

# Your C++ code
MY_SRC = $(XO_ROOT_SRC)/examples/HelloWorld.cpp

FREETYPE_SRC = \
$(XO_ROOT_SRC)/dependencies/freetype/src/autofit/autofit.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftbase.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftbitmap.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/bdf/bdf.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/cff/cff.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/cache/ftcache.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftgasp.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftglyph.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/gzip/ftgzip.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftinit.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftlcdfil.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/lzw/ftlzw.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftstroke.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftsystem.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/smooth/smooth.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftbbox.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftmm.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftpfr.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftsynth.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/fttype1.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/base/ftwinfnt.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/pcf/pcf.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/pfr/pfr.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/psaux/psaux.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/pshinter/pshinter.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/psnames/psmodule.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/raster/raster.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/sfnt/sfnt.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/truetype/truetype.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/type1/type1.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/cid/type1cid.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/type42/type42.c \
$(XO_ROOT_SRC)/dependencies/freetype/src/winfonts/winfnt.c \

XO_SRC = \
$(XO_ROOT_SRC)/xo/Android/xoLibJni.cpp \
$(XO_ROOT_SRC)/xo/xoDefs.cpp \
$(XO_ROOT_SRC)/xo/xoDoc.cpp \
$(XO_ROOT_SRC)/xo/xoDocUI.cpp \
$(XO_ROOT_SRC)/xo/xoDocGroup.cpp \
$(XO_ROOT_SRC)/xo/xoEvent.cpp \
$(XO_ROOT_SRC)/xo/xoMem.cpp \
$(XO_ROOT_SRC)/xo/xoMsgLoop.cpp \
$(XO_ROOT_SRC)/xo/xoPlatform.cpp \
$(XO_ROOT_SRC)/xo/xoString.cpp \
$(XO_ROOT_SRC)/xo/xoStringTable.cpp \
$(XO_ROOT_SRC)/xo/xoStyle.cpp \
$(XO_ROOT_SRC)/xo/xoSysWnd.cpp \
$(XO_ROOT_SRC)/xo/xoTags.cpp \
$(XO_ROOT_SRC)/xo/Canvas/xoCanvas2D.cpp \
$(XO_ROOT_SRC)/xo/Dom/xoDomEl.cpp \
$(XO_ROOT_SRC)/xo/Dom/xoDomCanvas.cpp \
$(XO_ROOT_SRC)/xo/Dom/xoDomNode.cpp \
$(XO_ROOT_SRC)/xo/Dom/xoDomText.cpp \
$(XO_ROOT_SRC)/xo/Image/xoImage.cpp \
$(XO_ROOT_SRC)/xo/Image/xoImageStore.cpp \
$(XO_ROOT_SRC)/xo/Layout/xoLayout.cpp \
$(XO_ROOT_SRC)/xo/Layout/xoLayout2.cpp \
$(XO_ROOT_SRC)/xo/Layout/xoTextLayout.cpp \
$(XO_ROOT_SRC)/xo/Parse/xoDocParser.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderBase.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderer.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderGL.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderGL_Defs.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderDoc.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderDomEl.cpp \
$(XO_ROOT_SRC)/xo/Render/xoRenderStack.cpp \
$(XO_ROOT_SRC)/xo/Render/xoStyleResolve.cpp \
$(XO_ROOT_SRC)/xo/Render/xoTextureAtlas.cpp \
$(XO_ROOT_SRC)/xo/Render/xoVertexTypes.cpp \
$(XO_ROOT_SRC)/xo/Text/xoFontStore.cpp \
$(XO_ROOT_SRC)/xo/Text/xoGlyphCache.cpp \
$(XO_ROOT_SRC)/xo/Text/xoTextDefs.cpp \
$(XO_ROOT_SRC)/xo/Shaders/Processed_glsl/FillShader.cpp \
$(XO_ROOT_SRC)/xo/Shaders/Processed_glsl/FillTexShader.cpp \
$(XO_ROOT_SRC)/xo/Shaders/Processed_glsl/RectShader.cpp \
$(XO_ROOT_SRC)/xo/Shaders/Processed_glsl/TextRGBShader.cpp \
$(XO_ROOT_SRC)/xo/Shaders/Processed_glsl/TextWholeShader.cpp \
$(XO_ROOT_SRC)/dependencies/agg/src/agg_vcgen_stroke.cpp \
$(XO_ROOT_SRC)/dependencies/agg/src/agg_vpgen_clip_polygon.cpp \
$(XO_ROOT_SRC)/dependencies/agg/src/agg_vpgen_clip_polyline.cpp \
$(XO_ROOT_SRC)/dependencies/hash/xxhash.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Containers/queue.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/cpu.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/err.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/filesystem.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/process.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/syncprims.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/timeprims.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Platform/thread.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Strings/ConvertUTF.cpp \
$(XO_ROOT_SRC)/dependencies/Panacea/Strings/fmt.cpp \
$(XO_ROOT_SRC)/dependencies/stb_image.cpp

LOCAL_C_INCLUDES := $(XO_ROOT_INCLUDES)/xo $(XO_ROOT_INCLUDES)/dependencies/freetype/include $(XO_ROOT_INCLUDES)/dependencies/agg/include
LOCAL_MODULE     := libxo
LOCAL_CFLAGS     := -Werror -DFT2_BUILD_LIBRARY
LOCAL_CPPFLAGS   := -Werror -std=c++11
LOCAL_SRC_FILES  := $(FREETYPE_SRC) $(XO_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
