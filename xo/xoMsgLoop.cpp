#include "pch.h"
#include "xoDefs.h"
#include "xoDocGroup.h"
#include "xoSysWnd.h"
#include "xoEvent.h"

#if XO_PLATFORM_WIN_DESKTOP

//#define MSGTRACE XOTRACE
#define MSGTRACE(...)

static bool AnyDocsDirty()
{
	for ( int i = 0; i < xoGlobal()->Docs.size(); i++ )
	{
		if ( xoGlobal()->Docs[i]->IsDocVersionDifferentToRenderer() )
			return true;
	}
	return false;
}

XOAPI void xoRunWin32MessageLoop()
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
			XOTIME("Render cold\n");
			MSGTRACE( "Render cold\n" );
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				break;
			MSGTRACE( "GetMessage returned\n" );
		}
		else
		{
			//XOTIME("Render hot\n");
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
		double nextFrameStart = lastFrameStart + 1.0 / xoGlobal()->TargetFPS;
		if ( now >= nextFrameStart || AnyDocsDirty() )
		{
			MSGTRACE( "Render enter\n" );
			renderIdle = true;
			lastFrameStart = now;
			for ( int i = 0; i < xoGlobal()->Docs.size(); i++ )
			{
				xoRenderResult rr = xoGlobal()->Docs[i]->Render();
				if ( rr != xoRenderResultIdle )
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

		xoProcessDocQueue();
	}

	//timeEndPeriod( 5 );
}

#elif XO_PLATFORM_LINUX_DESKTOP

extern xoSysWnd* SingleMainWnd;

// This implementation is a super-quick get-something-on-the-screen kinda thing.
// To be consistent with Windows Desktop we should process events on a different
// thread to the render thread. Also, we blindly send events to all docs, which
// is absurd.
XOAPI void xoRunXMessageLoop()
{
	xoProcessDocQueue();

	while(1)
	{
		XEvent xev;
		XNextEvent( SingleMainWnd->XDisplay, &xev );
        
		if ( xev.type == Expose )
		{
			XWindowAttributes wa;
			XGetWindowAttributes( SingleMainWnd->XDisplay, SingleMainWnd->XWindow, &wa );
			/*
			glXMakeCurrent( SingleMainWnd->XDisplay, SingleMainWnd->Window, SingleMainWnd->GLContext );
			glViewport( 0, 0, gwa.width, gwa.height );
			//DrawAQuad(); 
			//glXSwapBuffers(dpy, win);
			glXMakeCurrent( SingleMainWnd->XDisplay, None, Null );
			*/
			xoEvent nev;
			nev.Type = xoEventWindowSize;
			nev.Points[0].x = wa.width;
			nev.Points[0].y = wa.height;
			for ( int i = 0; i < xoGlobal()->Docs.size(); i++ )
				xoGlobal()->Docs[i]->ProcessEvent( nev );

		}
		else if ( xev.type == KeyPress )
		{
			XOTRACE( "key = %d\n", xev.xkey.keycode );
			if ( xev.xkey.keycode == 24 ) // 'q'
				break;
		}
		else if ( xev.type == MotionNotify )
		{
			//XOTRACE( "x,y = %d,%d\n", xev.xmotion.x, xev.xmotion.y );
			xoEvent nev;
			nev.Type = xoEventMouseMove;
			nev.Points[0].x = xev.xmotion.x;
			nev.Points[0].y = xev.xmotion.y;
			for ( int i = 0; i < xoGlobal()->Docs.size(); i++ )
				xoGlobal()->Docs[i]->ProcessEvent( nev );
		}

		for ( int i = 0; i < xoGlobal()->Docs.size(); i++ )
		{
			xoRenderResult rr = xoGlobal()->Docs[i]->Render();
			//XOTRACE( "rr = %d\n", rr );
			//if ( rr != xoRenderResultIdle )
			//	renderIdle = false;
		}

		xoProcessDocQueue();
	}
}

#endif
