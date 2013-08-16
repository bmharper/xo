LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

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
../../../nudom/Render/nuRenderGL.cpp \
../../../nudom/Render/nuRenderDoc.cpp \
../../../nudom/Render/nuRenderDomEl.cpp \
../../../nudom/Render/nuRenderStack.cpp \
../../../nudom/Render/nuStyleResolve.cpp \
../../../nudom/Text/nuTextCache.cpp \
../../../nudom/Text/nuTextDefs.cpp \
../../../dependencies/Panacea/Containers/queue.cpp \
../../../dependencies/Panacea/Platform/cpu.cpp \
../../../dependencies/Panacea/Platform/err.cpp \
../../../dependencies/Panacea/Platform/syncprims.cpp \
../../../dependencies/Panacea/Platform/thread.cpp \
../../../dependencies/Panacea/Strings/fmt.cpp \

MY_SRC = ../../HelloWorld/HelloWorld.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../nudom
LOCAL_MODULE     := libnudom
LOCAL_CFLAGS     := -Werror -std=c++11
LOCAL_SRC_FILES  := $(NU_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2


include $(BUILD_SHARED_LIBRARY)
