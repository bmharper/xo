#pragma once

// This uses .png images as the "truth" of a test
class xoImageTester
{
public:
	xoImageTester();
	~xoImageTester();

	static void		DoDirectory(const char* dir);
	static void		DoTruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup);
	static xo::String	PathRelativeToTestData(const char* path, const char* extension = nullptr);

	void		SetSize(uint32_t width, uint32_t height);

	void		VerifyWithImage(const char* filename, std::function<void(xo::DomNode& root)> setup);
	void		CreateTruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup);

	// Intended to be the "universal" function that does a verify, and then optionally prompts to write
	// a new version of the truth. If you're running in unattended mode though, then the test simply fails.
	void		TruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup);

	// assumes RGBA8
	static bool	ImageEquals(uint32_t width1, uint32_t height1, int stride1, const void* data1, uint32_t width2, uint32_t height2, int stride2, const void* data2);

protected:
	xo::SysWnd*	Wnd;
	int			ImageWidth = 0;
	int			ImageHeight = 0;

	void		CreateOrVerifyTruthImage(bool create, const char* filename, std::function<void(xo::DomNode& root)> setup);
};
