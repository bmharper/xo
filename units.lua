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

local xo = SharedLibrary {
	Name = "xo",
	Libs = { 
		{ "opengl32.lib", "user32.lib", "gdi32.lib", "winmm.lib" ; Config = "win*" },
		{ "X11", "GL", "GLU", "stdc++"; Config = "linux-*" },
	},
	SourceDir = ".",
	Includes = {
		"xo",
		"dependencies/freetype/include",
		"dependencies/agg/include",
	},
	Depends = { crt, freetype, directx, },
	PrecompiledHeader = {
		Source = "xo/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		Glob { Extensions = { ".h" }, Dir = "xo", },
		Glob { Extensions = { ".h" }, Dir = "dependencies", Recursive = false },
		Glob { Extensions = { ".h" }, Dir = "dependencies/Panacea", },
		"xo/xoDefs.cpp",
		"xo/xoDoc.cpp",
		"xo/xoDocUI.cpp",
		"xo/xoDocGroup.cpp",
		{ "xo/xoDocGroup_Windows.cpp"; Config = "win*" },
		"xo/xoEvent.cpp",
		"xo/xoMem.cpp",
		"xo/xoMsgLoop.cpp",
		"xo/xoPlatform.cpp",
		"xo/xoString.cpp",
		"xo/xoStringTable.cpp",
		"xo/xoStyle.cpp",
		"xo/xoSysWnd.cpp",
		"xo/xoTags.cpp",
		"xo/Canvas/xoCanvas2D.cpp",
		"xo/Dom/xoDomEl.cpp",
		"xo/Dom/xoDomCanvas.cpp",
		"xo/Dom/xoDomNode.cpp",
		"xo/Dom/xoDomText.cpp",
		"xo/Image/xoImage.cpp",
		"xo/Image/xoImageStore.cpp",
		"xo/Layout/xoLayout.cpp",
		"xo/Layout/xoLayout2.cpp",
		"xo/Layout/xoLayout3.cpp",
		"xo/Layout/xoTextLayout.cpp",
		"xo/Parse/xoDocParser.cpp",
		"xo/Render/xoRenderBase.cpp",
		"xo/Render/xoRenderDX.cpp",
		"xo/Render/xoRenderDX_Defs.cpp",
		"xo/Render/xoRenderer.cpp",
		"xo/Render/xoRenderGL.cpp",
		"xo/Render/xoRenderGL_Defs.cpp",
		"xo/Render/xoRenderDoc.cpp",
		"xo/Render/xoRenderDomEl.cpp",
		"xo/Render/xoRenderStack.cpp",
		"xo/Render/xoStyleResolve.cpp",
		"xo/Render/xoTextureAtlas.cpp",
		"xo/Render/xoVertexTypes.cpp",
		"xo/Text/xoFontStore.cpp",
		"xo/Text/xoGlyphCache.cpp",
		"xo/Text/xoTextDefs.cpp",
		--"xo/Shaders/Processed_glsl/CurveShader.cpp",
		"xo/Shaders/Processed_glsl/FillShader.cpp",
		"xo/Shaders/Processed_glsl/FillTexShader.cpp",
		"xo/Shaders/Processed_glsl/RectShader.cpp",
		"xo/Shaders/Processed_glsl/TextRGBShader.cpp",
		"xo/Shaders/Processed_glsl/TextWholeShader.cpp",
		"xo/Shaders/Processed_hlsl/FillShader.cpp",
		"xo/Shaders/Processed_hlsl/FillTexShader.cpp",
		"xo/Shaders/Processed_hlsl/RectShader.cpp",
		"xo/Shaders/Processed_hlsl/TextRGBShader.cpp",
		"xo/Shaders/Processed_hlsl/TextWholeShader.cpp",
		--"xo/Shaders/Helpers/xoPreprocessor.cpp",
		"dependencies/agg/src/agg_vcgen_stroke.cpp",
		"dependencies/agg/src/agg_vpgen_clip_polygon.cpp",
		"dependencies/agg/src/agg_vpgen_clip_polyline.cpp",
		"dependencies/hash/xxhash.cpp",
		"dependencies/Panacea/Containers/queue.cpp",
		"dependencies/Panacea/Platform/cpu.cpp",
		"dependencies/Panacea/Platform/err.cpp",
		"dependencies/Panacea/Platform/filesystem.cpp",
		"dependencies/Panacea/Platform/process.cpp",
		"dependencies/Panacea/Platform/syncprims.cpp",
		"dependencies/Panacea/Platform/timeprims.cpp",
		"dependencies/Panacea/Platform/thread.cpp",
		"dependencies/Panacea/Platform/ConvertUTF.cpp",
		"dependencies/Panacea/Strings/fmt.cpp",
		"dependencies/GL/gl_xo.cpp",
		{ "dependencies/GL/wgl_xo.cpp"; Config = "win*" },
		{ "dependencies/GL/glx_xo.cpp"; Config = "linux-gcc-debug-default" },
		"dependencies/stb_image.cpp",
	},
}

local HelloAmalgamation = Program {
	Name = "HelloAmalgamation",
	Libs = { 
		{ "opengl32.lib", "user32.lib", "gdi32.lib", "winmm.lib" ; Config = "win*" },
		{ "X11", "GL", "GLU", "stdc++"; Config = "linux-*" },
	},
	Defines = {
		"XO_AMALGAMATION"
	},
	Depends = {
		crtStatic,
		directx,
	},
	Sources = {
		"amalgamation/xo-amalgamation.cpp",
		"amalgamation/xo-amalgamation-freetype.c",
		"templates/xoWinMain.cpp",
		"examples/HelloAmalgamation/HelloAmalgamation.cpp",
	}
}

local function XoExampleApp(template, example)
	return Program {
		Name = example,
		Includes = { "xo" },
		Libs = { "stdc++", "m"; Config = "linux-*" },
		Depends = {
			crt,
			xo
		},
		Sources = {
			"templates/" .. template,
			"examples/" .. example .. ".cpp",
		}
	}
end

local ExampleBench       = XoExampleApp("xoWinMain.cpp", "Bench")
local ExampleCanvas      = XoExampleApp("xoWinMain.cpp", "Canvas")
local ExampleEvents      = XoExampleApp("xoWinMain.cpp", "Events")
local ExampleHelloWorld  = XoExampleApp("xoWinMain.cpp", "HelloWorld")
local ExampleKitchenSink = XoExampleApp("xoWinMain.cpp", "KitchenSink")
local ExampleLowLevel    = XoExampleApp("xoWinMainLowLevel.cpp", "RunAppLowLevel")

local Test = Program {
	Name = "Test",
	SourceDir = ".",
	Depends = {
		xo,
		crt,
	},
	Libs = { 
		{ "m", "stdc++"; Config = "linux-*" },
	},
	PrecompiledHeader = {
		Source = "tests/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		Glob { Extensions = { ".h" }, Dir = "tests", },
		-- The following platform mismatch is purely because of the mismatch of
		-- precompiled header behaviour on linux vs windows
		{ "dependencies/stb_image.cpp"; Config = "win*" },
		{ "dependencies/stb_image.c"; Config = "linux-*" },
		"dependencies/stb_image_write.h",
		"tests/test.cpp",
		"tests/test_DocumentClone.cpp",
		"tests/test_Layout.cpp",
		--"tests/test_Preprocessor.cpp",
		"tests/test_Stats.cpp",
		"tests/test_String.cpp",
		"tests/test_Styles.cpp",
		"tests/xoImageTester.cpp",
	}
}

Default(xo)
Default(Test)
Default(ExampleBench)
Default(ExampleCanvas)
Default(ExampleEvents)
Default(ExampleHelloWorld)
Default(ExampleKitchenSink)
Default(ExampleLowLevel)

