#include "../../nuDom/nuDom.h"

#ifdef _WIN32
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
#else
int main( int argc, char** args )
#endif
{
	nuInitialize();

	nuSysWnd* wnd = nuSysWnd::Create();
	wnd->Attach( new nuDoc(), true );
	nuDoc* doc = wnd->Doc();
	
	int nx = 100;
	int ny = 100;
	for ( int y = 0; y < ny; y++ )
	{
		nuDomEl* line = new nuDomEl();
		line->Tag = nuTagDiv;
		line->Style.Parse( "width: 100%; height: 1%; margin: 1px; display: block;" );
		doc->Root.Children += line;
		for ( int x = 0; x < nx; x++ )
		{
			nuDomEl* div = new nuDomEl();
			div->Tag = nuTagDiv;
			div->Style.Parse( "width: 1%; height: 100%; border-radius: 2px; margin: 1px; display: inline;" );
			div->Style.Parse( "background: #e00e" );
			div->Style.SetBackgroundColor( nuColor::RGBA(x * 5, 0, 0, 255) );
			line->Children += div;
		}
	}

	wnd->Show();
	//nuRunEventLoop();
	delete wnd;
	nuShutdown();

	return 0;
}
