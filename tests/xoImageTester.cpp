#include "pch.h"
#include "xoImageTester.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // convert from __int64 to int
#endif
#include "../dependencies/stb_image_write.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

xoImageTester::xoImageTester()
{
	Wnd = xo::SysWnd::CreateWithDoc(0);
	xo::AddOrRemoveDocsFromGlobalList();
	SetSize(256, 256);
	Wnd->Show();
}

xoImageTester::~xoImageTester()
{
	delete Wnd;
	xo::AddOrRemoveDocsFromGlobalList();
}

void xoImageTester::DoDirectory(const char* dir)
{
	struct Item
	{
		xo::String	Path;
		int			ImageSize;
	};
	xo::cheapvec<Item> files;
	auto found = [&files](const xo::FilesystemItem& item)
	{
		if (xo::TempString(item.Name).EndsWith(".xoml"))
		{
			int imageSize = 256;
			xo::String root = item.Root;
			xo::String fullName = xo::String(item.Root) + XO_DIR_SEP_STR + item.Name;
			// interpret example-32px.xoml to mean the output image is 32 x 32 pixels
			if (fullName.EndsWith("px.xoml"))
			{
				size_t slash = fullName.RIndex("-");
				XO_ASSERT(slash != -1);
				imageSize = atoi(fullName.SubStr(slash + 1, fullName.Length() - 7).Z); // 7 = length of "px.xoml"
			}
			files += Item{ fullName, imageSize };
		}
		return true;
	};
	xo::FindFiles(PathRelativeToTestData(dir).Z, found);

	xoImageTester tester;

	for (size_t i = 0; i < files.size(); i++)
	{
		Item& item = files[i];
		printf("Test %3d %30s\n", (int) i, item.Path.Z);
		auto setup = [&item](xo::DomNode& root)
		{
			xo::String err = root.Parse(LoadFileAsString(item.Path.Z).Z);
			XO_ASSERT(err == "");
		};
		tester.SetSize(item.ImageSize, item.ImageSize);
		tester.TruthImage(item.Path.Z, setup);
	}
}

void xoImageTester::DoTruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup)
{
	xoImageTester t;
	t.TruthImage(filename, setup);
}

xo::String xoImageTester::PathRelativeToTestData(const char* path, const char* extension)
{
	// binPath: C:\dev\individual\xo\t2-output\win64-msvc2013-debug-default\Test.exe
	// result:  C:\dev\individual\xo\testdata\<path>
	std::string binPath = TTGetProcessPath();
	xo::String fullPath = binPath.c_str();
	auto parts = fullPath.Split(XO_DIR_SEP_STR);
	parts.pop();
	parts.pop();
	parts.pop();
	fullPath = xo::String::Join(parts, XO_DIR_SEP_STR);
	fullPath += xo::String(XO_DIR_SEP_STR) + "testdata";
	if (path[0] != 0 || (extension && extension[0] != 0))
	{
		fullPath += XO_DIR_SEP_STR;
		fullPath += path;
	}
	if (extension != nullptr)
		fullPath += extension;
	return fullPath;
}

void xoImageTester::SetSize(uint32_t width, uint32_t height)
{
	if (width == ImageWidth && height == ImageHeight)
		return;

	// Note that the following technique does not work when the OS enforces a minimum window size.
	// Windows does this if you use a regular window. But we don't use a regular window - we use
	// a window without any border or any caption.

	// This sets the non-client rectangle, but we want our client size to be width,height
	int sampleSize = 200;
	Wnd->SetPosition(xo::Box(0, 0, sampleSize, sampleSize), xo::SysWnd::SetPosition_Size);
	xo::Box client = Wnd->GetRelativeClientRect();
	// Compensate by making non-client larger
	uint32_t necessaryWidth = width + (sampleSize - client.Width());
	uint32_t necessaryHeight = height + (sampleSize - client.Height());
	Wnd->SetPosition(xo::Box(0, 0, necessaryWidth, necessaryHeight), xo::SysWnd::SetPosition_Size);

	client = Wnd->GetRelativeClientRect();

	ImageWidth = width;
	ImageHeight = height;
}

void xoImageTester::TruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup)
{
	// The plan is to have an interactive GUI here someday where you get presented
	// with the failing image pair, and you can choose whether to mark the new one as "correct".
	// Until that happens, just temporarily change the value of this constant
	// in order to write a new truth image.
	bool overwrite_DO_NOT_COMMIT_THIS_CHANGE = false; // << FALSE << THIS MUST ALWAYS BE FALSE WHEN YOU COMMIT
	CreateOrVerifyTruthImage(overwrite_DO_NOT_COMMIT_THIS_CHANGE, filename, setup);
}

void xoImageTester::VerifyWithImage(const char* filename, std::function<void(xo::DomNode& root)> setup)
{
	CreateOrVerifyTruthImage(false, filename, setup);
}

void xoImageTester::CreateTruthImage(const char* filename, std::function<void(xo::DomNode& root)> setup)
{
	CreateOrVerifyTruthImage(true, filename, setup);
}

bool xoImageTester::ImageEquals(uint32_t width1, uint32_t height1, int stride1, const void* data1, uint32_t width2, uint32_t height2, int stride2, const void* data2)
{
	if (width1 != width2 ||
			height1 != height2)
		return false;

	const int32_t thresholdMax = 0;
	const int32_t thresholdMin = -thresholdMax;
	for (int32_t y = 0; y < (int32_t) height1; y++)
	{
		uint8_t* line1 = ((uint8_t*) data1) + y * stride1;
		uint8_t* line2 = ((uint8_t*) data2) + y * stride2;
		for (uint32_t x = 0; x < width1; x++, line1 += 4, line2 += 4)
		{
			int32_t dr = (int32_t) line1[0] - (int32_t) line2[0];
			int32_t dg = (int32_t) line1[1] - (int32_t) line2[1];
			int32_t db = (int32_t) line1[2] - (int32_t) line2[2];
			int32_t da = (int32_t) line1[3] - (int32_t) line2[3];
			int ok = 1;
			ok &= dr >= thresholdMin && dr <= thresholdMax;
			ok &= dg >= thresholdMin && dg <= thresholdMax;
			ok &= db >= thresholdMin && db <= thresholdMax;
			ok &= da >= thresholdMin && da <= thresholdMax;
			if (!ok)
				return false;
		}
	}
	return true;
}

void xoImageTester::CreateOrVerifyTruthImage(bool create, const char* filename, std::function<void(xo::DomNode& root)> setup)
{
	// populate the document
	Wnd->DocGroup->Doc->Reset();
	setup(Wnd->DocGroup->Doc->Root);

	// if filename is not rooted, then assume it's relative to 'testdata'
	xo::String fixedRoot = filename;
	if (filename[0] != '/' && filename[0] != '\\' && filename[1] != ':')
		fixedRoot = PathRelativeToTestData(filename);

	xo::String truthFile = fixedRoot;
	xo::String newSample = fixedRoot + "-observed-result";
	truthFile += ".png";
	newSample += ".png";

	xo::Image img;
	xo::RenderResult res = Wnd->DocGroup->RenderToImage(img);
	if (create)
	{
		stbi_write_png(truthFile.Z, img.GetWidth(), img.GetHeight(), 4, img.TexDataAtLine(0), img.TexStride);
	}
	else
	{
		int width = 0, height = 0, comp = 0;
		unsigned char* data = stbi_load(truthFile.Z, &width, &height, &comp, 4);
		bool same = ImageEquals(img.GetWidth(), img.GetHeight(), img.TexStride, img.TexDataAtLine(0), width, height, width * 4, data);
		if (!same)
		{
			stbi_write_png(newSample.Z, img.GetWidth(), img.GetHeight(), 4, img.TexDataAtLine(0), img.TexStride);
		}
		TTASSERT(same);
		stbi_image_free(data);
	}
}
