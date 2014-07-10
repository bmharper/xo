require 'tundra.syntax.glob'

local winDebugFilter = "win*-*-debug"
local winReleaseFilter = "win*-*-release"
local directxFilter = "win*"

local winKernelLibs = { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "advapi32.lib", "shell32.lib", "comctl32.lib", 
						"uuid.lib", "ole32.lib", "oleaut32.lib", "shlwapi.lib", "OLDNAMES.lib", "wldap32.lib", "wsock32.lib",
						"Psapi.lib", "Msimg32.lib", "Comdlg32.lib", "RpcRT4.lib", "Iphlpapi.lib", "Delayimp.lib" }

-- Dynamic (msvcr110.dll etc) CRT linkage
local winLibsDynamicCRTDebug = tundra.util.merge_arrays( { "msvcrtd.lib", "msvcprtd.lib", "comsuppwd.lib" }, winKernelLibs )
local winLibsDynamicCRTRelease = tundra.util.merge_arrays( { "msvcrt.lib", "msvcprt.lib", "comsuppw.lib" }, winKernelLibs )

winLibsDynamicCRTDebug.Config = winDebugFilter
winLibsDynamicCRTRelease.Config = winReleaseFilter

-- Static CRT linkage
local winLibsStaticCRTDebug = tundra.util.merge_arrays( { "libcmtd.lib", "libcpmtd.lib", "comsuppwd.lib" }, winKernelLibs )
local winLibsStaticCRTRelease = tundra.util.merge_arrays( { "libcmt.lib", "libcpmt.lib", "comsuppw.lib" }, winKernelLibs )

winLibsStaticCRTDebug.Config = winDebugFilter
winLibsStaticCRTRelease.Config = winReleaseFilter

local winDynamicOpts = {
	{ "/MDd";					Config = winDebugFilter },
	{ "/MD";					Config = winReleaseFilter },
}

local winStaticOpts = {
	{ "/MTd";					Config = winDebugFilter },
	{ "/MT";					Config = winReleaseFilter },
}

local winDefs = {
	{ "_CRTDBG_MAP_ALLOC";		Config = winDebugFilter },
}

local winDynamicEnv = {
	CCOPTS = winDynamicOpts,
	CXXOPTS = winDynamicOpts,
	CCDEFS = winDefs,
	CPPDEFS = winDefs,
}

local winStaticEnv = {
	CCOPTS = winStaticOpts,
	CXXOPTS = winStaticOpts,
	CCDEFS = winDefs,
	CPPDEFS = winDefs,
}

local crtDynamic = ExternalLibrary {
	Name = "crtdynamic",
	Propagate = {
		Env = winDynamicEnv,
		Libs = {
			winLibsDynamicCRTDebug,
			winLibsDynamicCRTRelease,
		},
	},
}

local crtStatic = ExternalLibrary {
	Name = "crtstatic",
	Propagate = {
		Env = winStaticEnv,
		Libs = {
			winLibsStaticCRTDebug,
			winLibsStaticCRTRelease,
		},
	},
}

-- Swapping this out will change linkage to use MSVCR120.dll and its cousins,
-- instead of statically linking to the required MSVC runtime libraries.
-- I don't understand why, but if we build with crtStatic, then tests fail
-- due to heap corruption. I can only guess it's the old issue of two different
-- libraries using different heap properties. For example, library A allocated
-- memory with debug_alloc(), and then library B tries to free that memory with
-- regular free(). However, I cannot figure out how that is happening.
--local crt = crtStatic
local crt = crtDynamic

local directx = ExternalLibrary {
	Name = "directx",
	Propagate = {
		Libs = {
			{ "D3D11.lib", "d3dcompiler.lib"; Config = directxFilter },
		},
	},
}

local unicode = ExternalLibrary {
	Name = "unicode",
	Propagate = {
		Defines = { "UNICODE", "_UNICODE" },
	},
}

local freetype = StaticLibrary {
	Name = "freetype",
	Defines = {
		"FT2_BUILD_LIBRARY",
	},
	Depends = { crt, },
	SourceDir = "dependencies/freetype",
	Includes = "dependencies/freetype/include",
	Sources = {
		"src/autofit/autofit.c",
		"src/base/ftbase.c",
		"src/base/ftbitmap.c",
		"src/bdf/bdf.c",
		"src/cff/cff.c",
		"src/cache/ftcache.c",
		"src/base/ftgasp.c",
		"src/base/ftglyph.c",
		"src/gzip/ftgzip.c",
		"src/base/ftinit.c",
		"src/base/ftlcdfil.c",
		"src/lzw/ftlzw.c",
		"src/base/ftstroke.c",
		"src/base/ftsystem.c",
		"src/smooth/smooth.c",
		"src/base/ftbbox.c",
		"src/base/ftmm.c",
		"src/base/ftpfr.c",
		"src/base/ftsynth.c",
		"src/base/fttype1.c",
		"src/base/ftwinfnt.c",
		"src/pcf/pcf.c",
		"src/pfr/pfr.c",
		"src/psaux/psaux.c",
		"src/pshinter/pshinter.c",
		"src/psnames/psmodule.c",
		"src/raster/raster.c",
		"src/sfnt/sfnt.c",
		"src/truetype/truetype.c",
		"src/type1/type1.c",
		"src/cid/type1cid.c",
		"src/type42/type42.c",
		"src/winfonts/winfnt.c",
	}	
}

--[[
local freetype_gl = StaticLibrary {
	Name = "freetype_gl",
	--Defines = {
	--	"",
	--},
	SourceDir = "dependencies/freetype-gl",
	Includes = "dependencies/freetype-gl",
	Sources = {
		"mat4.c",
		"platform.c",
		"shader.c",
		"text-buffer.c",
		"texture-atlas.c",
		"texture-font.c",
		"vector.c",
		"vertex-attribute.c",
		"vertex-buffer.c",
	}	
}
--]]

-- This is not used right now - shaders are hand-written in both environments
--[[
local hlslang = Program {
	Name = "hlslang",
	SourceDir = ".",
	Env = {
		CXXOPTS = {
			{ "/wd4267"; Config = "win*" },			-- loss of data
			{ "/wd4018"; Config = "win*" },			-- signed/unsigned mismatch
		}
	},
	Defines = {
		"_CRT_SECURE_NO_WARNINGS",
	},
	Includes = {
		"dependencies/hlslang/src/MachineIndependent",
		{ "dependencies/hlslang/src/OSDependent/Windows"; Config = "win*" },
	},
	Depends = { crt, },
	Sources = {
		Glob { Extensions = { ".cpp", ".c" }, Dir = "dependencies/hlslang/src/GLSLCodeGen", },
		Glob { Extensions = { ".cpp", ".c" }, Dir = "dependencies/hlslang/src/MachineIndependent", Recursive = true },
		{ Glob { Extensions = { ".cpp", ".c" }, Dir = "dependencies/hlslang/src/OSDependent/Windows" }, Config = "win*" },
		"dependencies/hlslang/src/hlslang.cpp",
	}
}
--]]

local nudom = SharedLibrary {
	Name = "nudom",
	Libs = { 
		{ "opengl32.lib", "user32.lib", "gdi32.lib", "winmm.lib" ; Config = "win*" },
		{ "X11", "GL", "GLU", "stdc++"; Config = "linux-*" },
	},
	SourceDir = ".",
	Includes = {
		"nudom",
		"dependencies/freetype/include"
	},
	Depends = { crt, freetype, directx, },
	PrecompiledHeader = {
		Source = "nudom/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		Glob { Extensions = { ".h" }, Dir = "nudom", },
		Glob { Extensions = { ".h" }, Dir = "dependencies", Recursive = false },
		Glob { Extensions = { ".h" }, Dir = "dependencies/Panacea", },
		"nudom/nuDefs.cpp",
		"nudom/nuDoc.cpp",
		"nudom/nuEvent.cpp",
		"nudom/nuMem.cpp",
		"nudom/nuMsgLoop.cpp",
		"nudom/nuPlatform.cpp",
		"nudom/nuString.cpp",
		"nudom/nuStringTable.cpp",
		"nudom/nuStyle.cpp",
		"nudom/nuStyleParser.cpp",
		"nudom/nuSysWnd.cpp",
		"nudom/nuDocGroup.cpp",
		{ "nudom/nuDocGroup_Windows.cpp"; Config = "win*" },
		"nudom/Dom/nuDomEl.cpp",
		"nudom/Dom/nuDomNode.cpp",
		"nudom/Dom/nuDomText.cpp",
		"nudom/Image/nuImage.cpp",
		"nudom/Image/nuImageStore.cpp",
		"nudom/Layout/nuLayout.cpp",
		"nudom/Layout/nuLayout2.cpp",
		"nudom/Layout/nuTextLayout.cpp",
		"nudom/Render/nuRenderBase.cpp",
		"nudom/Render/nuRenderDX.cpp",
		"nudom/Render/nuRenderDX_Defs.cpp",
		"nudom/Render/nuRenderer.cpp",
		"nudom/Render/nuRenderGL.cpp",
		"nudom/Render/nuRenderGL_Defs.cpp",
		"nudom/Render/nuRenderDoc.cpp",
		"nudom/Render/nuRenderDomEl.cpp",
		"nudom/Render/nuRenderStack.cpp",
		"nudom/Render/nuStyleResolve.cpp",
		"nudom/Render/nuTextureAtlas.cpp",
		"nudom/Render/nuVertexTypes.cpp",
		"nudom/Text/nuFontStore.cpp",
		"nudom/Text/nuGlyphCache.cpp",
		"nudom/Text/nuTextDefs.cpp",
		"nudom/Shaders/Processed_glsl/CurveShader.cpp",
		"nudom/Shaders/Processed_glsl/FillShader.cpp",
		"nudom/Shaders/Processed_glsl/FillTexShader.cpp",
		"nudom/Shaders/Processed_glsl/RectShader.cpp",
		"nudom/Shaders/Processed_glsl/TextRGBShader.cpp",
		"nudom/Shaders/Processed_glsl/TextWholeShader.cpp",
		"nudom/Shaders/Processed_hlsl/FillShader.cpp",
		"nudom/Shaders/Processed_hlsl/RectShader.cpp",
		"nudom/Shaders/Processed_hlsl/TextRGBShader.cpp",
		"nudom/Shaders/Processed_hlsl/TextWholeShader.cpp",
		--"nudom/Shaders/Helpers/nuPreprocessor.cpp",
		"dependencies/Panacea/Containers/queue.cpp",
		"dependencies/Panacea/Platform/cpu.cpp",
		"dependencies/Panacea/Platform/err.cpp",
		"dependencies/Panacea/Platform/process.cpp",
		"dependencies/Panacea/Platform/syncprims.cpp",
		"dependencies/Panacea/Platform/thread.cpp",
		"dependencies/Panacea/Strings/ConvertUTF.cpp",
		"dependencies/Panacea/Strings/fmt.cpp",
		"dependencies/GL/gl_nudom.cpp",
		{ "dependencies/GL/wgl_nudom.cpp"; Config = "win*" },
		{ "dependencies/GL/glx_nudom.cpp"; Config = "linux-gcc-debug-default" },
		"dependencies/stb_image.cpp",
	},
}

local HelloWorld = Program {
	Name = "HelloWorld",
	Includes = { "nudom" },
	--Libs = { "X11", "GL", "GLU", "stdc++", "pthread", "rt"; Config = "linux-*" },
	Libs = { "stdc++"; Config = "linux-*" },
	Depends = {
		crt,
		nudom
	},
	Sources = {
		"templates/nuWinMain.cpp",
		"samples/HelloWorld/HelloWorld.cpp",
	}
}

--[[
local HelloAmalgamation = Program {
	Name = "HelloAmalgamation",
	Includes = { "nudom" },
	Depends = {
		crt,
	},
	Sources = {
		"amalgamation/nuDom-amalgamation.cpp",
		"templates/nuWinMain.cpp",
		"samples/HelloAmalgamation/HelloAmalgamation.cpp",
	}
}
--]]

local KitchenSink = Program {
	Name = "KitchenSink",
	Includes = { "nudom" },
	Depends = {
		crt,
		nudom
	},
	Sources = {
		"templates/nuWinMain.cpp",
		"samples/KitchenSink/KitchenSink.cpp",
	}
}

local Bench = Program {
	Name = "Bench",
	Includes = { "nudom" },
	Depends = {
		crt,
		nudom
	},
	Sources = {
		"templates/nuWinMain.cpp",
		"samples/Bench/Bench.cpp",
	}
}

local tinytest = StaticLibrary {
	Name = "tinytest",
	SourceDir = "dependencies/TinyTest",
	Includes = { "dependencies", "dependencies/TinyTest" },
	Depends = {
		crt,
		unicode,
	},
	Sources = {
		"Tiny.cpp",
		{ "StackWalker.cpp"; Config ="win*" },
	}
}

local tinytest_app = Program {
	Name = "tinytest_app",
	SourceDir = "dependencies/TinyTest",
	Includes = { "dependencies", "dependencies/TinyTest" },
	Depends = {
		crt,
		unicode,
	},
	Sources = {
		"Tiny.cpp",
		"TinyTest.cpp",
		"dependencies.cpp",
		{ "StackWalker.cpp"; Config ="win*" },
	}
}

local Test = Program {
	Name = "Test",
	SourceDir = ".",
	--Includes = { "nudom" },
	Depends = {
		nudom,
		crt,
		tinytest,
	},
	PrecompiledHeader = {
		Source = "tests/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		Glob { Extensions = { ".h" }, Dir = "tests", },
		"dependencies/stb_image.cpp",
		"dependencies/stb_image_write.h",
		"tests/test.cpp",
		"tests/test_DocumentClone.cpp",
		"tests/test_Layout.cpp",
		"tests/test_Preprocessor.cpp",
		"tests/test_Stats.cpp",
		"tests/test_String.cpp",
		"tests/test_Styles.cpp",
		"tests/nuImageTester.cpp",
	}
}

Default(nudom)
Default(HelloWorld)
Default(Bench)
Default(Test)
Default(tinytest_app)
