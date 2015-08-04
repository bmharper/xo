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
	Wnd = xoSysWnd::CreateWithDoc(0);
	xoAddOrRemoveDocsFromGlobalList();
	SetSize(256, 256);
	Wnd->Show();
}

xoImageTester::~xoImageTester()
{
	delete Wnd;
	xoAddOrRemoveDocsFromGlobalList();
}

void xoImageTester::DoDirectory(const char* dir)
{
	struct Item
	{
		xoString	Path;
		int			ImageSize;
	};
	podvec<Item> files;
	auto found = [&files](const AbcFilesystemItem& item)
	{
		if (xoTempString(item.Name).EndsWith(".xoml"))
		{
			int imageSize = 256;
			xoString root = item.Root;
			xoString fullName = xoString(item.Root) + ABC_DIR_SEP_STR + item.Name;
			// interpret example-32px.xoml to mean the output image is 32 x 32 pixels
			if (fullName.EndsWith("px.xoml"))
			{
				intp slash = fullName.RIndex("-");
				XOASSERT(slash != -1);
				imageSize = atoi(fullName.SubStr(slash + 1, fullName.Length() - 7).Z); // 7 = length of "px.xoml"
			}
			files += Item{ fullName, imageSize };
		}
		return true;
	};
	AbcFilesystemFindFiles(PathRelativeToTestData(dir).Z, found);

	xoImageTester tester;

	for (intp i = 0; i < files.size(); i++)
	{
		Item& item = files[i];
		printf("Test %3d %30s\n", (int) i, item.Path.Z);
		auto setup = [&item](xoDomNode& root)
		{
			xoString err = root.Parse(LoadFileAsString(item.Path.Z).Z);
			XOASSERT(err == "");
		};
		tester.SetSize(item.ImageSize, item.ImageSize);
		tester.TruthImage(item.Path.Z, setup);
	}
}

void xoImageTester::DoTruthImage(const char* filename, std::function<void(xoDomNode& root)> setup)
{
	xoImageTester t;
	t.TruthImage(filename, setup);
}

xoString xoImageTester::PathRelativeToTestData(const char* path, const char* extension)
{
	// binPath: C:\dev\individual\xo\t2-output\win64-msvc2013-debug-default\Test.exe
	// result:  C:\dev\individual\xo\testdata\<path>
	char binPath[2048];
	AbcProcessGetPath(binPath, arraysize(binPath));
	xoString fullPath = binPath;
	auto parts = fullPath.Split(ABC_DIR_SEP_STR);
	parts.pop();
	parts.pop();
	parts.pop();
	fullPath = xoString::Join(parts, ABC_DIR_SEP_STR);
	fullPath += xoString(ABC_DIR_SEP_STR) + "testdata";
	if (path[0] != 0 || (extension && extension[0] != 0))
	{
		fullPath += ABC_DIR_SEP_STR;
		fullPath += path;
	}
	if (extension != nullptr)
		fullPath += extension;
	return fullPath;
}

void xoImageTester::SetSize(u32 width, u32 height)
{
	if (width == ImageWidth && height == ImageHeight)
		return;

	// Note that the following technique does not work when the OS enforces a minimum window size.
	// Windows does this if you use a regular window. But we don't use a regular window - we use
	// a window without any border or any caption.

	// This sets the non-client rectangle, but we want our client size to be width,height
	int sampleSize = 200;
	Wnd->SetPosition(xoBox(0, 0, sampleSize, sampleSize), xoSysWnd::SetPosition_Size);
	xoBox client = Wnd->GetRelativeClientRect();
	// Compensate by making non-client larger
	u32 necessaryWidth = width + (sampleSize - client.Width());
	u32 necessaryHeight = height + (sampleSize - client.Height());
	Wnd->SetPosition(xoBox(0, 0, necessaryWidth, necessaryHeight), xoSysWnd::SetPosition_Size);

	client = Wnd->GetRelativeClientRect();

	ImageWidth = width;
	ImageHeight = height;
}

void xoImageTester::TruthImage(const char* filename, std::function<void(xoDomNode& root)> setup)
{
	// The plan is to have an interactive GUI here someday where you get presented
	// with the failing image pair, and you can choose whether to mark the new one as "correct".
	// Until that happens, just temporarily change the value of this constant
	// in order to write a new truth image.
	bool overwrite_DO_NOT_COMMIT_THIS_CHANGE = false; // << FALSE << THIS MUST ALWAYS BE FALSE WHEN YOU COMMIT
	CreateOrVerifyTruthImage(overwrite_DO_NOT_COMMIT_THIS_CHANGE, filename, setup);
}

void xoImageTester::VerifyWithImage(const char* filename, std::function<void(xoDomNode& root)> setup)
{
	CreateOrVerifyTruthImage(false, filename, setup);
}

void xoImageTester::CreateTruthImage(const char* filename, std::function<void(xoDomNode& root)> setup)
{
	CreateOrVerifyTruthImage(true, filename, setup);
}

bool xoImageTester::ImageEquals(u32 width1, u32 height1, int stride1, const void* data1, u32 width2, u32 height2, int stride2, const void* data2)
{
	if (width1 != width2 ||
			height1 != height2)
		return false;

	const int32 thresholdMax = 0;
	const int32 thresholdMin = -thresholdMax;
	for (int32 y = 0; y < (int32) height1; y++)
	{
		uint8* line1 = ((uint8*) data1) + y * stride1;
		uint8* line2 = ((uint8*) data2) + y * stride2;
		for (u32 x = 0; x < width1; x++, line1 += 4, line2 += 4)
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
			if (!ok)
				return false;
		}
	}
	return true;
}

void xoImageTester::CreateOrVerifyTruthImage(bool create, const char* filename, std::function<void(xoDomNode& root)> setup)
{
	// populate the document
	Wnd->DocGroup->Doc->Reset();
	setup(Wnd->DocGroup->Doc->Root);

	// if filename is not rooted, then assume it's relative to 'testdata'
	xoString fixedRoot = filename;
	if (filename[0] != '/' && filename[0] != '\\' && filename[1] != ':')
		fixedRoot = PathRelativeToTestData(filename);

	xoString truthFile = fixedRoot;
	xoString newSample = fixedRoot + "-observed-result";
	truthFile += ".png";
	newSample += ".png";

	xoImage img;
	xoRenderResult res = Wnd->DocGroup->RenderToImage(img);
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
