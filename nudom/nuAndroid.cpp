#include "pch.h"
#include "nuDefs.h"
#include "nuDoc.h"
#include "nuRender.h"
#include "nuRenderGL.h"
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
    JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_destroy(JNIEnv * env, jint iskilling);
    JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_step(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_input(JNIEnv * env, jobject obj, jint type, jfloatArray x, jfloatArray y);
};

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    LOGI("init 1 (%d, %d)", width, height);
	if ( !Initialized )
	{
		Initialized = true;
	    
		LOGI("init 2");
		nuInitialize();

		LOGI("init 3");
		nuMain( nuMainEventInit );
	    
		LOGI("init 4");

		//LOGI("init 3");
		//Proc = new nuProcessor();
		//Proc->Doc = new nuDoc();

		//Proc->Doc->Root.Style.Parse( "background: #fff;" );

		/*
		LOGI("init 4");
		nuDoc* doc = Proc->Doc;
		for ( int i = 0; i < 4; i++ )
		{
			nuDomEl* div = new nuDomEl();
			div->Tag = nuTagDiv;
			div->Style.Parse( "width: 100px; height: 100px; border-radius: 15px; display: inline;" );
			div->Style.Parse( "margin: 3px;" );
			doc->Root.Children += div;
		}
		doc->Root.Children[0]->Style.Parse( "background: #e00e" );
		doc->Root.Children[1]->Style.Parse( "background: #0e0e" );
		doc->Root.Children[2]->Style.Parse( "background: #00ee" );
		doc->Root.Children[3]->Style.Parse( "background: #aeaa" );
		*/

		//LOGI("init 5");
		//RGL = new nuRenderGL();
		//RGL->CreateShaders();
		//LOGI("init 6");
	}
	
	if ( MainWnd )
	{
		MainWnd->Processor->Doc->WindowWidth = width;
		MainWnd->Processor->Doc->WindowHeight = height;
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

JNIEXPORT void JNICALL Java_com_android_nudom_NuLib_step(JNIEnv * env, jobject obj)
{
	if ( MainWnd )
	{
		//LOGI("render 1 %d %d", Proc->Doc->WindowWidth, Proc->Doc->WindowHeight );
		MainWnd->Processor->RenderDoc->UpdateDoc( *MainWnd->Processor->Doc );

		//LOGI("render 2");
		MainWnd->Processor->RenderDoc->Render( MainWnd->RGL );

		//LOGI("render 3");
	}
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
	    LOGI("dispatching touch input");

		nuEvent ev;
		ev.Type = nuEventTouch;
		ev.PointCount = n;
		for ( int i = 0; i < n; i++ )
		{
			ev.Points[i].x = xe[i];
			ev.Points[i].y = ye[i];
		}
		MainWnd->Processor->BubbleEvent( ev );
	}

    env->ReleaseFloatArrayElements( x, xe, 0 );
	env->ReleaseFloatArrayElements( y, ye, 0 );
}
