#include "pch.h"
#include "nuDefs.h"
#include "nuDoc.h"
#include "Render/nuRenderer.h"
#include "Render/nuRenderGL.h"
#include "Render/nuRenderDoc.h"
#include "nuProcessor.h"
#include "nuEvent.h"
#include "nuSysWnd.h"
#include <stdlib.h>

#define  LOG_TAG    "nudom"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

//static float		RED = 0;
//static float		GREEN = 0;
//static float		BLUE = 0;
static bool			Initialized = false;
//static nuRenderGL*	RGL = NULL;
//static nuProcessor*	Proc = NULL;
extern nuSysWnd*	MainWnd;
void nuMain( nuMainEvent ev );

#define CLAMP(a,mmin,mmax) (((a) < (mmin) ? (mmin) : ((a) > (mmax) ? (mmax) : (a))))

extern "C" {
    JNIEXPORT void	JNICALL Java_com_android_nudom_NuLib_init(JNIEnv * env, jobject obj,  jint width, jint height);
	JNIEXPORT void	JNICALL Java_com_android_nudom_NuLib_surfacelost(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_nudom_NuLib_destroy(JNIEnv * env, jint iskilling);
    JNIEXPORT int	JNICALL Java_com_android_nudom_NuLib_step(JNIEnv * env, jobject obj);
    JNIEXPORT void	JNICALL Java_com_android_nudom_NuLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y);
};

// Right now our Android applications can have only one window, 
// but we might have more windows if we were doing things such as
// notifications or widgets.
static nuEvent MakeEvent()
{
	nuEvent e;
	e.Processor = MainWnd->Processor;
	return e;
}

static void ProcessAllEvents()
{
	while ( true )
	{
		if ( !AbcSemaphoreWait( nuGlobal()->EventQueue.SemaphoreObj(), 0 ) )
			break;
		nuEvent ev;
		NUVERIFY( nuGlobal()->EventQueue.PopTail( ev ) );
		ev.Processor->ProcessEvent( ev );
	}
}

// Our current Android model uses the Java Activity as the entry point, which means
// we do not have control of the main application message loop. This means that our event
// queue is effectively short-circuited. We always post an event, and then immediately consume it.
// It is good to keep it in this style, because it means that in future we can switch
// to a purely native application, where we DO control the message loop, and we want
// to process UI on a dedicated thread, the same as we do for Win32.
static void PostEvent( const nuEvent& ev )
{
	nuGlobal()->EventQueue.Add( ev );
	ProcessAllEvents();
}

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_init(JNIEnv * env, jobject obj, jint width, jint height)
{
    LOGI("NuLib_init 1 (%d, %d)", width, height);
	if ( !Initialized )
	{
		Initialized = true;
	    
		LOGI("NuLib_init 2");
		nuInitialize();

		LOGI("NuLib_init 3");
		nuMain( nuMainEventInit );
	    
		LOGI("NuLib_init 4");
	}
	
	if ( MainWnd )
	{
		nuEvent ev = MakeEvent();
		ev.MakeWindowSize( width, height );
		PostEvent( ev );
		LOGI("NuLib_init 5");
	}
}

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_destroy(JNIEnv * env, jint iskilling)
{
	if ( Initialized )
	{
		Initialized = false;
	    LOGI("destroy");
		nuMain( nuMainEventShutdown );
		nuShutdown();
	}
}

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_surfacelost(JNIEnv * env, jobject obj)
{
	if ( MainWnd )
		MainWnd->SurfaceLost();
}

JNIEXPORT int JNICALL Java_com_android_nudom_NuLib_step(JNIEnv * env, jobject obj)
{
	if ( MainWnd )
	{
		//LOGI("render 1 %d %d", Proc->Doc->WindowWidth, Proc->Doc->WindowHeight );
		//MainWnd->Processor->RenderDoc->CopyFromCanonical( *MainWnd->Processor->Doc );

		//LOGI("render 2");
		//MainWnd->Processor->RenderDoc->Render( MainWnd->RGL );

		//LOGI("render 3");
		return MainWnd->Processor->Render();
	}
	return nuRenderResultNeedMore;
}

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y)
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
	if ( MainWnd )
	{
	    LOGI("dispatching touch input 123");

		nuEvent ev = MakeEvent();
		ev.Type = nuEventTouch;
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
