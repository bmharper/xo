#include "nuDom.h"
#include "nuRender.h"
#include "nuProcessor.h"

// This file should be compiled and linked into your exe

#if NU_WIN_DESKTOP

void nuMain( nuMainEvent ev );

// add or remove documents that are queued for addition or removal
static void ProcessDocQueue()
{
	nuProcessor* p = NULL;

	while ( p = nuGlobal()->DocRemoveQueue.PopTailR() )
		erase_delete( nuGlobal()->Docs, nuGlobal()->Docs.find(p) );

	while ( p = nuGlobal()->DocAddQueue.PopTailR() )
		nuGlobal()->Docs += p;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	nuInitialize();

	nuMain( nuMainEventInit );

	double lastFrameStart = AbcTimeAccurateRTSeconds();

	// Toggled when all renderers report that they have no further work to do (ie no animations playing, now or any time in the future)
	// In that case, the only way we can have something happen is if we have an incoming message.
	// Of course, this is NOT true for IO that is busy occuring on the UI thread.
	// We'll have to devise a way (probably just a process-global custom window message) of causing the main loop to wake up.
	bool renderIdle = false;

	while ( true )
	{
		// When idle, use GetMessage so that the OS can put us into a good sleep
		MSG msg;
		bool haveMsg = true;
		if ( renderIdle )
		{
			//NUTRACE( "Render cold\n" );
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				break;
		}
		else
		{
			//NUTRACE( "Render hot\n" );
			haveMsg = !!PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
		}
		
		if ( haveMsg )
		{
			if ( msg.message == WM_QUIT )
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		double now = AbcTimeAccurateRTSeconds();
		double nextFrameStart = lastFrameStart + 1.0 / nuGlobal()->TargetFPS;
		if ( now >= nextFrameStart )
		{
			renderIdle = true;
			lastFrameStart = now;
			for ( int i = 0; i < nuGlobal()->Docs.size(); i++ )
			{
				nuRenderResult rr = nuGlobal()->Docs[i]->Render();
				if ( rr != nuRenderResultIdle )
					renderIdle = false;
			}
		}
		else if ( !renderIdle )
		{
			// assume that thread time slices are on the order of our target FPS
			AbcSleep( 0 );
		}

		ProcessDocQueue();
	}

	nuMain( nuMainEventShutdown );

	nuShutdown();
	return 0;
}

#endif
