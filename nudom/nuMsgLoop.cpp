#include "pch.h"
#include "nuDefs.h"
#include "nuDocGroup.h"
#include "nuSysWnd.h"

#if NU_PLATFORM_WIN_DESKTOP

//#define MSGTRACE NUTRACE
#define MSGTRACE(...)

static bool AnyDocsDirty()
{
	for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
	{
		if ( nuGlobal()->Docs[i]->IsDocVersionDifferentToRenderer() )
			return true;
	}
	return false;
}

NUAPI void nuRunWin32MessageLoop()
{
	const double HEAT_TIME = 0.3;
	double lastFrameStart = AbcTimeAccurateRTSeconds();
	double lastHeatAt = AbcTimeAccurateRTSeconds();

	// Toggled when all renderers report that they have no further work to do (ie no animations playing, now or any time in the future)
	// In that case, the only way we can have something happen is if we have an incoming message.
	// Of course, this is NOT true for IO that is busy occurring on the UI thread.
	// We'll have to devise a way (probably just a process-global custom window message) of causing the main loop to wake up.
	bool renderIdle = false;
	
	// Trying various things to get latency down to the same level as GDI, but I just can't do it.
	//timeBeginPeriod( 5 );

	while ( true )
	{
		// When idle, use GetMessage so that the OS can put us into a good sleep
		MSG msg;
		bool haveMsg = true;
		if ( renderIdle && !AnyDocsDirty() && AbcTimeAccurateRTSeconds() - lastHeatAt > HEAT_TIME )
		{
			NUTIME("Render cold\n");
			MSGTRACE( "Render cold\n" );
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				break;
			MSGTRACE( "GetMessage returned\n" );
		}
		else
		{
			//NUTIME("Render hot\n");
			MSGTRACE( "Render hot\n" );
			haveMsg = !!PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
		}
		
		if ( haveMsg )
		{
			MSGTRACE( "msg start: %x\n", msg.message );
			if ( msg.message == WM_QUIT )
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			MSGTRACE( "msg end: %x\n", msg.message );
			if ( msg.message != WM_TIMER )
				lastHeatAt = AbcTimeAccurateRTSeconds();
		}

		double now = AbcTimeAccurateRTSeconds();
		double nextFrameStart = lastFrameStart + 1.0 / nuGlobal()->TargetFPS;
		if ( now >= nextFrameStart || AnyDocsDirty() )
		{
			MSGTRACE( "Render enter\n" );
			renderIdle = true;
			lastFrameStart = now;
			for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
			{
				nuRenderResult rr = nuGlobal()->Docs[i]->Render();
				if ( rr != nuRenderResultIdle )
					renderIdle = false;
			}
		}
		else
		{
			if ( AbcTimeAccurateRTSeconds() - lastHeatAt > 0.050 )
				AbcSleep(5);
			else
				AbcSleep(0);
		}

		nuProcessDocQueue();
	}

	//timeEndPeriod( 5 );
}

#elif NU_PLATFORM_LINUX_DESKTOP

extern nuSysWnd* SingleMainWnd;

NUAPI void nuRunXMessageLoop()
{
	while(1)
	{
		XEvent xev;
		XNextEvent( SingleMainWnd->XDisplay, &xev );
        
		if ( xev.type == Expose )
		{
			/*
			XWindowAttributes wa;
			XGetWindowAttributes( SingleMainWnd->XDisplay, SingleMainWnd->Window, &wa );
			glXMakeCurrent( SingleMainWnd->XDisplay, SingleMainWnd->Window, SingleMainWnd->GLContext );
			glViewport( 0, 0, gwa.width, gwa.height );
			//DrawAQuad(); 
			//glXSwapBuffers(dpy, win);
			glXMakeCurrent( SingleMainWnd->XDisplay, None, Null );
			*/
		}
		else if ( xev.type == KeyPress )
		{
			NUTRACE( "key = %d\n", xev.xkey.keycode );
			for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
			{
				nuRenderResult rr = nuGlobal()->Docs[i]->Render();
				//if ( rr != nuRenderResultIdle )
				//	renderIdle = false;
			}
		}
	}
}

#endif