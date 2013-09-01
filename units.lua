require 'tundra.syntax.glob'

local winKernelLibs = { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "advapi32.lib", "shell32.lib", "comctl32.lib", 
						"uuid.lib", "ole32.lib", "oleaut32.lib", "shlwapi.lib", "OLDNAMES.lib", "wldap32.lib", "wsock32.lib",
						"Psapi.lib", "Msimg32.lib", "Comdlg32.lib", "RpcRT4.lib", "Iphlpapi.lib", "Delayimp.lib" }

local winDllCRTDebug = tundra.util.merge_arrays( { "msvcrtd.lib", "msvcprtd.lib", "comsuppwd.lib" }, winKernelLibs )
local winDllCRTRelease = tundra.util.merge_arrays( { "msvcrt.lib", "msvcprt.lib", "comsuppw.lib" }, winKernelLibs )

local winDebugFilter = "win*-*-debug"
local winReleaseFilter = "win*-*-release"

winDllCRTDebug.Config = winDebugFilter
winDllCRTRelease.Config = winReleaseFilter

local winDllOpts = {
	{ "/MDd";					Config = winDebugFilter },
	{ "/MD";					Config = winReleaseFilter },
}

local winDllDefs = {
	{ "_CRTDBG_MAP_ALLOC";		Config = winDebugFilter },
}

local winDllEnv = {
	CCOPTS = winDllOpts,
	CXXOPTS = winDllOpts,
	CCDEFS = winDllDefs,
	CPPDEFS = winDllDefs,
}

local crt = ExternalLibrary {
	Name = "crt",
	Propagate = {
		Env = winDllEnv,
		Libs = {
			winDllCRTDebug,
			winDllCRTRelease,
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

local nudom = SharedLibrary {
	Name = "nudom",
	Libs = { "opengl32.lib", "user32.lib", "gdi32.lib", "winmm.lib", "freetype.lib", },
	SourceDir = ".",
	Includes = {
		"nudom",
		"dependencies/freetype/include"
	},
	Depends = { crt, freetype, },
	PrecompiledHeader = {
		Source = "nudom/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		--[[
		FGlob {
			Dir = "nudom",
			Extensions = { ".cpp", ".h" },
			Recursive = true,
			Filters = {
				{ Pattern = "_Windows", Config = "win*" },
				--{ Pattern = "pch", Config = "ignore" },
			}
		},
		--Glob { Extensions = { ".h" }, Recursive = true, Dir = "../dependencies" },
		--]]
		Glob { Extensions = { ".h" }, Recursive = true, Dir = ".", },
		"nudom/nuDefs.cpp",
		"nudom/nuDoc.cpp",
		"nudom/nuDomEl.cpp",
		"nudom/nuEvent.cpp",
		"nudom/nuLayout.cpp",
		"nudom/nuMem.cpp",
		"nudom/nuMsgLoop_Windows.cpp",
		"nudom/nuPlatform.cpp",
		"nudom/nuString.cpp",
		"nudom/nuStringTable.cpp",
		"nudom/nuStyle.cpp",
		"nudom/nuStyleParser.cpp",
		"nudom/nuSysWnd.cpp",
		"nudom/nuDocGroup.cpp",
		"nudom/nuDocGroup_Windows.cpp",
		"nudom/Image/nuImage.cpp",
		"nudom/Image/nuImageStore.cpp",
		"nudom/Render/nuRenderer.cpp",
		"nudom/Render/nuRenderGL.cpp",
		"nudom/Render/nuRenderDoc.cpp",
		"nudom/Render/nuRenderDomEl.cpp",
		"nudom/Render/nuRenderStack.cpp",
		"nudom/Render/nuStyleResolve.cpp",
		"nudom/Render/nuTextureAtlas.cpp",
		"nudom/Text/nuFontStore.cpp",
		"nudom/Text/nuGlyphCache.cpp",
		"nudom/Text/nuTextDefs.cpp",
		"dependencies/Panacea/Containers/queue.cpp",
		"dependencies/Panacea/Platform/cpu.cpp",
		"dependencies/Panacea/Platform/err.cpp",
		"dependencies/Panacea/Platform/syncprims.cpp",
		"dependencies/Panacea/Platform/thread.cpp",
		"dependencies/Panacea/Strings/fmt.cpp",
		"dependencies/glext.cpp",
	},
}

local HelloWorld = Program {
	Name = "HelloWorld",
	Includes = { "nudom" },
	Depends = {
		crt,
		nudom
	},
	Sources = {
		"templates/nuWinMain.cpp",
		"samples/HelloWorld/HelloWorld.cpp",
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
		"tests/test.cpp",
		"tests/test_DocumentClone.cpp",
		"tests/test_Stats.cpp",
		"tests/test_Styles.cpp",
	}
}

Default(nudom)
Default(HelloWorld)
Default(Bench)
Default(Test)
Default(tinytest_app)
