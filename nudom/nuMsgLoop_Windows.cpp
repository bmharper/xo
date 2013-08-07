#include "pch.h"
#include "nuDefs.h"
#include "nuDocGroup.h"

#if NU_WIN_DESKTOP

//#define MSGTRACE NUTRACE
#define MSGTRACE(...)

static bool AnyDocsDirty()
{
	for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
	{
		if ( nuGlobal()->Docs[i]->IsDocNewerThanRenderer() )
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

	while ( true )
	{
		// When idle, use GetMessage so that the OS can put us into a good sleep
		MSG msg;
		bool haveMsg = true;
		if ( renderIdle && !AnyDocsDirty() && AbcTimeAccurateRTSeconds() - lastHeatAt > HEAT_TIME )
		{
			MSGTRACE( "Render cold\n" );
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				break;
			MSGTRACE( "GetMessage returned\n" );
		}
		else
		{
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
}

#endif