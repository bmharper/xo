#include "../xo/pch.h"
#include "../xo/xoDefs.h"
#include "../xo/xoDoc.h"
#include "../xo/Render/xoRenderer.h"
#include "../xo/Render/xoRenderGL.h"
#include "../xo/Render/xoRenderDoc.h"
#include "../xo/xoDocGroup.h"
#include "../xo/xoEvent.h"
#include "../xo/xoSysWnd.h"
#include <stdlib.h>

#define  LOG_TAG    "xo"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static bool			Initialized = false;
extern xoSysWnd*	SingleMainWnd;			// This is defined inside xoSysWnd.cpp

// This is defined inside one of your app-specific .cpp files. It is your only entry point.
void xoMain( xoSysWnd* wnd );

void xoMain( xoMainEvent ev )
{
	switch (ev)
	{
	case xoMainEventInit:
		//SingleMainWnd = xoSysWnd::CreateWithDoc();
		//xoMain( SingleMainWnd );
		break;
	case xoMainEventShutdown:
		//delete SingleMainWnd;
		break;
	}
}

#define CLAMP(a,mmin,mmax) (((a) < (mmin) ? (mmin) : ((a) > (mmax) ? (mmax) : (a))))

extern "C" {
	JNIEXPORT jint	JNICALL	JNI_OnLoad(JavaVM* vm, void* reserved);
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_initXo(JNIEnv * env, jobject obj, jstring cacheDir, jfloat scaledDensity);
	JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_initSurface(JNIEnv * env, jobject obj, jint width, jint height, jfloat scaledDensity);
	JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv * env, jobject obj, jint iskilling);
    JNIEXPORT int	JNICALL Java_com_android_xo_XoLib_render(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y);
};

// Right now our Android applications can have only one window, 
// but we might have more windows if we were doing things such as
// notifications or widgets.
static xoOriginalEvent MakeEvent()
{
	xoOriginalEvent e;
	e.DocGroup = SingleMainWnd->DocGroup;
	return e;
}

static void ProcessAllEvents()
{
	while ( true )
	{
		xoProcessDocQueue();
		if ( !AbcSemaphoreWait( xoGlobal()->EventQueue.SemaphoreObj(), 0 ) )
			break;
		xoOriginalEvent ev;
		XOVERIFY( xoGlobal()->EventQueue.PopTail( ev ) );
		XOTRACE( "ProcessEvent %p", ev.DocGroup );
		ev.DocGroup->ProcessEvent( ev.Event );
	}
}

// Our current Android model uses the Java Activity as the entry point, which means
// we do not have control of the main application message loop. This means that our event
// queue is effectively short-circuited. We always post an event, and then immediately consume it.
// It is good to keep it in this style, because it means that in future we can switch
// to a purely native application, where we DO control the message loop, and we want
// to process UI on a dedicated thread, the same as we do for Win32 and X11.
static void PostEvent( const xoOriginalEvent& ev )
{
	xoGlobal()->EventQueue.Add( ev );
	ProcessAllEvents();
}

static xoString GetString( JNIEnv* env, jstring jstr )
{
	const char* buf = env->GetStringUTFChars( jstr, nullptr );
	xoString copy;
	if ( buf != nullptr )
	{
		copy = buf;
		env->ReleaseStringUTFChars( jstr, buf );
	}
	return copy;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if ( vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK )
        return -1;

    LOGI("JNI_OnLoad");

    // Get jclass with env->FindClass.
    // Register methods with env->RegisterNatives.

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initXo(JNIEnv * env, jobject obj, jstring cacheDir, jfloat scaledDensity)
{
    LOGI("XoLib_initXo 1");
	if ( !Initialized )
	{
		Initialized = true;
	    
		LOGI("XoLib_init scaledDensity = %f", scaledDensity);

		xoInitParams ip;
		ip.EpToPixel = scaledDensity;
		ip.CacheDir = GetString( env, cacheDir );

		LOGI("XoLib_init 2");
		xoInitialize( &ip );

		LOGI("XoLib_init 3");
		xoMain( xoMainEventInit );	// this is no-op
	    
		LOGI("XoLib_init 4");
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_initSurface(JNIEnv * env, jobject obj, jint width, jint height, jfloat scaledDensity)
{
    LOGI("XoLib_initSurface 1 (%d, %d, %f)", width, height, scaledDensity);

	if ( !Initialized )
	{
		LOGE("XoLib_initSurface called, but initXo has not yet been called (or we have been shutdown)");
		return;
	}
	
	if ( SingleMainWnd == nullptr )
		SingleMainWnd = xoSysWnd::CreateWithDoc();

	if ( SingleMainWnd != nullptr )
	{
		SingleMainWnd->RelativeClientRect = xoBox( 0, 0, width, height );
		xoOriginalEvent ev = MakeEvent();
		ev.Event.MakeWindowSize( width, height );
		LOGI("XoLib_initSurface 4");
		PostEvent( ev );
		LOGI("XoLib_initSurface 5");
		xoMain( SingleMainWnd );
		LOGI("XoLib_initSurface 6");
	}
	else
	{
		LOGE("XoLib_initSurface: SingleMainWnd is null");
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv * env, jobject obj, jint iskilling)
{
	if ( Initialized )
	{
		Initialized = false;
	    LOGI("destroy");
		xoMain( xoMainEventShutdown );	// this is a no-op
		xoShutdown();
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv * env, jobject obj)
{
	LOGI("XoLib_surfacelost");
	if ( SingleMainWnd )
		SingleMainWnd->SurfaceLost();
}

JNIEXPORT int JNICALL Java_com_android_xo_XoLib_render(JNIEnv * env, jobject obj)
{
	if ( SingleMainWnd )
	{
		//LOGI("render 1 %d %d", Proc->Doc->WindowWidth, Proc->Doc->WindowHeight );
		//SingleMainWnd->DocGroup->RenderDoc->CopyFromCanonical( *SingleMainWnd->DocGroup->Doc );

		//LOGI("render 2");
		//SingleMainWnd->DocGroup->RenderDoc->Render( SingleMainWnd->RGL );

		//LOGI("XoLib.render");
		xoRenderResult r = SingleMainWnd->DocGroup->Render();
		return r;
	}
	return xoRenderResultNeedMore;
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y)
{
    jfloat* xe = env->GetFloatArrayElements( x, 0 );
	jfloat* ye = env->GetFloatArrayElements( y, 0 );
	int n = env->GetArrayLength( x );
	
	/*
	if ( n > 0 )   LOGI( "cool %d. element 0: %f %f\n", n, xe[0], ye[0] );
    else           LOGI( "none\n" );
    if ( n >= 1 )   RED = fmod( xe[0], 300.0f ) / 300.0f;
    if ( n >= 2 )   GREEN = fmod( xe[0], 300.0f ) / 300.0f;
    if ( n >= 3 )   BLUE = fmod( xe[0], 300.0f ) / 300.0f;
	*/
	if ( SingleMainWnd )
	{
	    //LOGI("dispatching touch input %d", type);

		xoOriginalEvent ev = MakeEvent();
		ev.Event.Type = xoEventTouch;
		ev.Event.PointCount = n;
		for ( int i = 0; i < n; i++ )
		{
			ev.Event.Points[i].x = xe[i];
			ev.Event.Points[i].y = ye[i];
		}
		PostEvent( ev );
	}

    env->ReleaseFloatArrayElements( x, xe, 0 );
	env->ReleaseFloatArrayElements( y, ye, 0 );
}
