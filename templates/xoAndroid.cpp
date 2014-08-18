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

//static float		RED = 0;
//static float		GREEN = 0;
//static float		BLUE = 0;
static bool			Initialized = false;
//static xoRenderGL*	RGL = NULL;
//static xoDocGroup*	Proc = NULL;
extern xoSysWnd*	SingleMainWnd;
void xoMain( xoMainEvent ev );

#define CLAMP(a,mmin,mmax) (((a) < (mmin) ? (mmin) : ((a) > (mmax) ? (mmax) : (a))))

extern "C" {
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_init(JNIEnv * env, jobject obj,  jint width, jint height, jfloat scaledDensity);
	JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv * env, jint iskilling);
    JNIEXPORT int	JNICALL Java_com_android_xo_XoLib_step(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_xo_XoLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y);
};

// Right now our Android applications can have only one window, 
// but we might have more windows if we were doing things such as
// notifications or widgets.
static xoEvent MakeEvent()
{
	xoEvent e;
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
		xoEvent ev;
		XOVERIFY( xoGlobal()->EventQueue.PopTail( ev ) );
		ev.DocGroup->ProcessEvent( ev );
	}
}

// Our current Android model uses the Java Activity as the entry point, which means
// we do not have control of the main application message loop. This means that our event
// queue is effectively short-circuited. We always post an event, and then immediately consume it.
// It is good to keep it in this style, because it means that in future we can switch
// to a purely native application, where we DO control the message loop, and we want
// to process UI on a dedicated thread, the same as we do for Win32.
static void PostEvent( const xoEvent& ev )
{
	xoGlobal()->EventQueue.Add( ev );
	ProcessAllEvents();
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_init(JNIEnv * env, jobject obj, jint width, jint height, jfloat scaledDensity)
{
    LOGI("XoLib_init 1 (%d, %d)", width, height);
	if ( !Initialized )
	{
		Initialized = true;
	    
		LOGI("XoLib_init 2");
		xoInitialize();

		LOGI("XoLib_init scaledDensity = %f", scaledDensity);
		xoGlobal()->EpToPixel = scaledDensity;

		LOGI("XoLib_init 3");
		xoMain( xoMainEventInit );
	    
		LOGI("XoLib_init 4");
	}
	
	if ( SingleMainWnd )
	{
		SingleMainWnd->RelativeClientRect = xoBox( 0, 0, width, height );
		xoEvent ev = MakeEvent();
		ev.MakeWindowSize( width, height );
		PostEvent( ev );
		LOGI("XoLib_init 5");
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_destroy(JNIEnv * env, jint iskilling)
{
	if ( Initialized )
	{
		Initialized = false;
	    LOGI("destroy");
		xoMain( xoMainEventShutdown );
		xoShutdown();
	}
}

JNIEXPORT void JNICALL Java_com_android_xo_XoLib_surfacelost(JNIEnv * env, jobject obj)
{
	LOGI("XoLib_surfacelost");
	if ( SingleMainWnd )
		SingleMainWnd->SurfaceLost();
}

JNIEXPORT int JNICALL Java_com_android_xo_XoLib_step(JNIEnv * env, jobject obj)
{
	if ( SingleMainWnd )
	{
		//LOGI("render 1 %d %d", Proc->Doc->WindowWidth, Proc->Doc->WindowHeight );
		//SingleMainWnd->DocGroup->RenderDoc->CopyFromCanonical( *SingleMainWnd->DocGroup->Doc );

		//LOGI("render 2");
		//SingleMainWnd->DocGroup->RenderDoc->Render( SingleMainWnd->RGL );

		//LOGI("render 3");
		int r = SingleMainWnd->DocGroup->Render();
		//LOGI("render done");
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

		xoEvent ev = MakeEvent();
		ev.Type = xoEventTouch;
		ev.PointCount = n;
		for ( int i = 0; i < n; i++ )
		{
			ev.Points[i].x = xe[i];
			ev.Points[i].y = ye[i];
		}
		PostEvent( ev );
	}

    env->ReleaseFloatArrayElements( x, xe, 0 );
	env->ReleaseFloatArrayElements( y, ye, 0 );
}
