#pragma once

// This uses .png images as the "truth" of a test
class nuImageTester
{
public:
				nuImageTester();
				~nuImageTester();

	void		SetSize( u32 width, u32 height );

	void		VerifyWithImage( const char* filename, std::function<void(nuDomEl& root)> setup );
	void		CreateTruthImage( const char* filename, std::function<void(nuDomEl& root)> setup );
	
	// Intended to be the "universal" function that does a verify, and then optionally prompts to write
	// a new version of the truth. If you're running in unattended mode though, then the test simply fails.
	void		TruthImage( const char* filename, std::function<void(nuDomEl& root)> setup );

	// assumes RGBA8
	static bool	ImageEquals( u32 width1, u32 height1, int stride1, const void* data1, u32 width2, u32 height2, int stride2, const void* data2 );

protected:
	nuSysWnd*	Wnd;

	void		CreateOrVerifyTruthImage( bool create, const char* filename, std::function<void(nuDomEl& root)> setup );
	nuString	FullPath( const char* path );
};
