#include "pch.h"
#include "nuImageTester.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4244) // convert from __int64 to int
#endif
#include "../dependencies/stb_image_write.h"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

nuImageTester::nuImageTester()
{
	Wnd = nuSysWnd::CreateWithDoc();
	nuProcessDocQueue();
	SetSize( 256, 256 );
	Wnd->Show();
}

nuImageTester::~nuImageTester()
{
	delete Wnd;
	nuProcessDocQueue();
}

void nuImageTester::SetSize( u32 width, u32 height )
{
	// This sets the non-client rectangle, but we want our client size to be width,height
	Wnd->SetPosition( nuBox(0, 0, width, height), nuSysWnd::SetPosition_Size );
	nuBox client = Wnd->GetRelativeClientRect();
	// Compensate by making non-client larger
	u32 nwidth = width + (width - client.Width());
	u32 nheight = height + (height - client.Height());
	if ( nwidth != width || nheight != height )
		Wnd->SetPosition( nuBox(0, 0, nwidth, nheight), nuSysWnd::SetPosition_Size );
}

void nuImageTester::TruthImage( const char* filename, std::function<void(nuDomNode& root)> setup )
{
	// The plan is to have an interactive GUI here someday where you get presented
	// with the failing image pair, and you can choose whether to mark the new one as "correct".
	bool overwrite_DO_NOT_COMMIT_THIS_CHANGE = false;
	CreateOrVerifyTruthImage( overwrite_DO_NOT_COMMIT_THIS_CHANGE, filename, setup );
}

void nuImageTester::VerifyWithImage( const char* filename, std::function<void(nuDomNode& root)> setup )
{
	CreateOrVerifyTruthImage( false, filename, setup );
}

void nuImageTester::CreateTruthImage( const char* filename, std::function<void(nuDomNode& root)> setup )
{
	CreateOrVerifyTruthImage( true, filename, setup );
}

bool nuImageTester::ImageEquals( u32 width1, u32 height1, int stride1, const void* data1, u32 width2, u32 height2, int stride2, const void* data2 )
{
	if (	width1 != width2 ||
			height1 != height2 )
		return false;

	const int32 thresholdMax = 0;
	const int32 thresholdMin = -thresholdMax;
	for ( int32 y = 0; y < (int32) height1; y++ )
	{
		uint8* line1 = ((uint8*) data1) + y * stride1;
		uint8* line2 = ((uint8*) data2) + y * stride2;
		for ( u32 x = 0; x < width1; x++, line1 += 4, line2 += 4 )
		{
			int32 dr = (int32) line1[0] - (int32) line2[0];
			int32 dg = (int32) line1[1] - (int32) line2[1];
			int32 db = (int32) line1[2] - (int32) line2[2];
			int32 da = (int32) line1[3] - (int32) line2[3];
			int ok = 1;
			ok &= dr >= thresholdMin && dr <= thresholdMax;
			ok &= dg >= thresholdMin && dg <= thresholdMax;
			ok &= db >= thresholdMin && db <= thresholdMax;
			ok &= da >= thresholdMin && da <= thresholdMax;
			if ( !ok )
				return false;
		}
	}
	return true;
}

void nuImageTester::CreateOrVerifyTruthImage( bool create, const char* filename, std::function<void(nuDomNode& root)> setup )
{
	// populate the document
	Wnd->DocGroup->Doc->Reset();
	setup( Wnd->DocGroup->Doc->Root );

	nuString truthFile = FullPath(filename);
	nuString newSample = FullPath((nuString(filename) + "-observed-result").Z);

	nuImage img;
	nuRenderResult res = Wnd->DocGroup->RenderToImage( img );
	if ( create )
	{
		stbi_write_png( truthFile.Z, img.GetWidth(), img.GetHeight(), 4, img.TexDataAtLine(0), img.TexStride );
	}
	else
	{
		int width = 0, height = 0, comp = 0;
		unsigned char* data = stbi_load( truthFile.Z, &width, &height, &comp, 4 );
		bool same = ImageEquals( img.GetWidth(), img.GetHeight(), img.TexStride, img.TexDataAtLine(0), width, height, width * 4, data );
		if ( !same )
		{
			stbi_write_png( newSample.Z, img.GetWidth(), img.GetHeight(), 4, img.TexDataAtLine(0), img.TexStride );
		}
		TTASSERT(same);
		stbi_image_free( data );
	}
}

nuString nuImageTester::FullPath( const char* path )
{
	// binPath: C:\dev\individual\nudom\t2-output\win64-msvc2013-debug-default\Test.exe
	// result:  C:/dev/individual/nudom/testdata/<path>
	char binPath[2048];
	AbcProcessGetPath( binPath, arraysize(binPath) );
	nuString fullPath = binPath;
	fullPath.ReplaceAll( "\\", "/" );
	auto parts = fullPath.Split( "/" );
	parts.pop();
	parts.pop();
	parts.pop();
	fullPath = nuString::Join( parts, "/" );
	fullPath += "/testdata/";
	fullPath += path;
	fullPath += ".png";
	return fullPath;
}
