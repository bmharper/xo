namespace xo {
#include "../xo/pch.h"
#include "../xo/Defs.h"
#include "../xo/Doc.h"
#include "../xo/Render/Renderer.h"
#include "../xo/Render/RenderGL.h"
#include "../xo/Render/RenderDoc.h"
#include "../xo/DocGroup.h"
#include "../xo/Event.h"
#include "../xo/SysWnd.h"
#include <stdlib.h>

#define LOG_TAG "xo"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace xo;

static bool    Initialized = false;
extern SysWnd* SingleMainWnd; // This is defined inside SysWnd.cpp

// This is defined inside one of your app-specific .cpp files. It is your only entry point.
void Main(SysWnd* wnd);

// I am keeping this structure here merely to remind myself that we might want to support
// a more complex application lifecycle than "one window"
void Main(MainEvent ev) {
	switch (ev) {
	case MainEventInit:
		//SingleMainWnd = SysWnd::CreateWithDoc();
		//Main( SingleMainWnd );
		break;
	case MainEventShutdown:
		//delete SingleMainWnd;
		break;
	}
}

#define CLAMP(a, mmin, mmax) (((a) < (mmin) ? (mmin) : ((a) > (mmax) ? (mmax) : (a))))

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved);
JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initXo(JNIEnv* env, jobject obj, jstring cacheDir, jfloat scaledDensity);
JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initSurface(JNIEnv* env, jobject obj, jint width, jint height, jfloat scaledDensity);
JNIEXPORT void JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv* env, jobject obj, jint iskilling);
JNIEXPORT int JNICALL Java_com_android_xo_XoLib_render(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_android_xo_XoLib_input(JNIEnv* env, jobject obj, jint type, jfloatArray x, jfloatArray y);
};

// Right now our Android applications can have only one window,
// but we might have more windows if we were doing things such as
// notifications or widgets.
static OriginalEvent MakeEvent() {
	OriginalEvent e;
	e.DocGroup = SingleMainWnd->DocGroup;
	return e;
}

static void ProcessAllEvents() {
	while (true) {
		ProcessDocQueue();
		if (!AbcSemaphoreWait(Global()->UIEventQueue.SemaphoreObj(), 0))
			break;
		OriginalEvent ev;
		XO_VERIFY(Global()->UIEventQueue.PopTail(ev));
		ev.DocGroup->ProcessEvent(ev.Event);
	}
}

// Our current Android model uses the Java Activity as the entry point, which means
// we do not have control of the main application message loop. This means that our event
// queue is effectively short-circuited. We always post an event, and then immediately consume it.
// It is good to keep it in this style, because it means that in future we can switch
// to a purely native application, where we DO control the message loop, and we want
// to process UI on a dedicated thread, the same as we do for Win32 and X11.
static void PostEvent(const OriginalEvent& ev) {
	Global()->UIEventQueue.Add(ev);
	ProcessAllEvents();
}

static String GetString(JNIEnv* env, jstring jstr) {
	const char* buf = env->GetStringUTFChars(jstr, nullptr);
	String      copy;
	if (buf != nullptr) {
		copy = buf;
		env->ReleaseStringUTFChars(jstr, buf);
	}
	return copy;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
		return -1;

	LOGI("JNI_OnLoad");

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initXo(JNIEnv* env, jobject obj, jstring cacheDir, jfloat scaledDensity) {
	LOGI("XoLib_initXo 1");
	if (!Initialized) {
		Initialized = true;

		LOGI("XoLib_init scaledDensity = %f", scaledDensity);

		InitParams ip;
		ip.EpToPixel = scaledDensity;
		ip.CacheDir  = GetString(env, cacheDir);

		LOGI("XoLib_init 2");
		Initialize(&ip);

		LOGI("XoLib_init 3");
		Main(MainEventInit); // this is no-op

		LOGI("XoLib_init 4");
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initSurface(JNIEnv* env, jobject obj, jint width, jint height, jfloat scaledDensity) {
	LOGI("XoLib_initSurface 1 (%d, %d, %f)", width, height, scaledDensity);

	if (!Initialized) {
		LOGE("XoLib_initSurface called, but initXo has not yet been called (or we have been shutdown)");
		return;
	}

	bool firstTime = SingleMainWnd == nullptr;

	if (firstTime)
		SingleMainWnd = SysWnd::CreateWithDoc();

	if (SingleMainWnd != nullptr) {
		SingleMainWnd->RelativeClientRect = Box(0, 0, width, height);
		OriginalEvent ev                  = MakeEvent();
		ev.Event.MakeWindowSize(width, height);
		LOGI("XoLib_initSurface posting window size event");
		PostEvent(ev);
		if (firstTime) {
			LOGI("XoLib_initSurface Main()");
			Main(SingleMainWnd);
		}
		LOGI("XoLib_initSurface done");
	} else {
		LOGE("XoLib_initSurface: SingleMainWnd is null");
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv* env, jobject obj, jint iskilling) {
	if (Initialized) {
		Initialized = false;
		LOGI("destroy");
		Main(MainEventShutdown); // this is a no-op
		Shutdown();
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv* env, jobject obj) {
	LOGI("XoLib_surfacelost");
	if (SingleMainWnd)
		SingleMainWnd->SurfaceLost();
}

JNIEXPORT int JNICALL Java_com_android_xo_XoLib_render(JNIEnv* env, jobject obj) {
	if (SingleMainWnd) {
		//LOGI("render 1 %d %d", Proc->Doc->WindowWidth, Proc->Doc->WindowHeight );
		//SingleMainWnd->DocGroup->RenderDoc->CopyFromCanonical( *SingleMainWnd->DocGroup->Doc );

		//LOGI("render 2");
		//SingleMainWnd->DocGroup->RenderDoc->Render( SingleMainWnd->RGL );

		//LOGI("XoLib.render");
		RenderResult r = SingleMainWnd->DocGroup->Render();
		return r;
	}
	return RenderResultNeedMore;
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_input(JNIEnv* env, jobject obj, jint type, jfloatArray x, jfloatArray y) {
	jfloat* xe = env->GetFloatArrayElements(x, 0);
	jfloat* ye = env->GetFloatArrayElements(y, 0);
	int     n  = env->GetArrayLength(x);

	/*
	if ( n > 0 )   LOGI( "cool %d. element 0: %f %f\n", n, xe[0], ye[0] );
	else           LOGI( "none\n" );
	if ( n >= 1 )   RED = fmod( xe[0], 300.0f ) / 300.0f;
	if ( n >= 2 )   GREEN = fmod( xe[0], 300.0f ) / 300.0f;
	if ( n >= 3 )   BLUE = fmod( xe[0], 300.0f ) / 300.0f;
	*/
	if (SingleMainWnd) {
		//LOGI("dispatching touch input %d", type);

		OriginalEvent ev    = MakeEvent();
		ev.Event.Type       = EventTouch;
		ev.Event.PointCount = n;
		for (int i = 0; i < n; i++) {
			ev.Event.Points[i].x = xe[i];
			ev.Event.Points[i].y = ye[i];
		}
		PostEvent(ev);
	}

	env->ReleaseFloatArrayElements(x, xe, 0);
	env->ReleaseFloatArrayElements(y, ye, 0);
}
}
