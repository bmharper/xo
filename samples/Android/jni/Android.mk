LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

NU_SRC = \
../../../nudom/nuDefs.cpp \
../../../nudom/nuDoc.cpp \
../../../nudom/nuDomEl.cpp \
../../../nudom/nuEvent.cpp \
../../../nudom/nuLayout.cpp \
../../../nudom/nuMem.cpp \
../../../nudom/nuProcessor.cpp \
../../../nudom/nuQueue.cpp \
../../../nudom/nuRender.cpp \
../../../nudom/nuRenderGL.cpp \
../../../nudom/nuString.cpp \
../../../nudom/nuStyle.cpp \
../../../nudom/nuSysWnd.cpp \
../../../nudom/nuAndroid.cpp \
../../../dependencies/Panacea/Containers/queue.cpp \
../../../dependencies/Panacea/Platform/err.cpp \
../../../dependencies/Panacea/Platform/syncprims.cpp \

MY_SRC = ../../HelloWorld/HelloWorld.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../nudom
LOCAL_MODULE     := libnudom
LOCAL_CFLAGS     := -Werror
LOCAL_SRC_FILES  := $(NU_SRC) $(MY_SRC)
LOCAL_LDLIBS     := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
