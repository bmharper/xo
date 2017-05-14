require 'tundra.syntax.glob'

local winFilter = "win*"
local winDebugFilter = "win*-*-debug"
local winReleaseFilter = "win*-*-release"
local linuxFilter = "linux-*-*-*"
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

-- Return an FGlob node that has our standard filters applied
local function makeGlob(dir, options)
	local filters = {
		{ Pattern = "_windows"; Config = winFilter },
		{ Pattern = "_linux"; Config = linuxFilter },
		{ Pattern = "_android"; Config = "ignore" },       -- Android stuff is built with a different build system
		{ Pattern = "[/\\]_[^/\\]*$"; Config = "ignore" }, -- Any file that starts with an underscore is ignored
	}
	if options.Ignore ~= nil then
		for _, ignore in ipairs(options.Ignore) do
			filters[#filters + 1] = { Pattern = ignore; Config = "ignore" }
		end
	end

	return FGlob {
		Dir = dir,
		Extensions = { ".c", ".cpp", ".h" },
		Filters = filters,
	}
end

local function merge_tables(a, b)
	local r = {}
	for k,v in pairs(a) do
		r[k] = v
	end
	for k,v in pairs(b) do
		r[k] = v
	end
	return r
end

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

local utfz = StaticLibrary {
	Name = "utfz",
	Depends = { crt, },
	Sources = {
		"dependencies/utfz/utfz.cpp",
	}
}

-- This is not used on Linux Desktop
local expat = StaticLibrary {
	Name = "expat",
	Depends = { crt, },
	Defines = {
		"XML_STATIC",
		{ "WIN32"; Config = winFilter },
	},
	Sources = {
		makeGlob("dependencies/expat", {})
	}
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

local xo_base = {
	Libs = { 
		{ "opengl32.lib", "user32.lib", "gdi32.lib", "winmm.lib" ; Config = "win*" },
		{ "X11", "GL", "GLU", "stdc++", "expat", "pthread"; Config = "linux-*" },
	},
	Defines = {
		"XML_STATIC", -- for expat
	},
	Includes = {
		"xo",
		"dependencies/freetype/include",
		"dependencies/agg/include",
		"dependencies/expat",
	},
	Depends = { crt, freetype, directx, utfz,
		{ expat; Config = winFilter },
	},
	PrecompiledHeader = {
		Source = "xo/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		makeGlob("xo", {}),
		makeGlob("dependencies/agg", {}),
		makeGlob("dependencies/ConvertUTF", {}),
		makeGlob("dependencies/GL", {}),
		makeGlob("dependencies/hash", {}),
	},
}

local xo = SharedLibrary(merge_tables(xo_base, {
		Name = "xo",
	}
))

local xo_static = StaticLibrary(merge_tables(xo_base, {
		Name = "xo_static",
	}
))

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

local function XoExampleApp(use_dynamic_xo, template, name, sources)
	local src = {
		"templates/" .. template,
		"dependencies/tsf/tsf.cpp",
	}
	for _, s in ipairs(sources) do
		src[#src + 1] = "examples/" .. s
	end

	local depends = { crt }
	local linux_libs = { "X11", "GL", "GLU", "stdc++", "m" }

	if use_dynamic_xo then
		depends[#depends + 1] = xo
	else
		depends[#depends + 1] = xo_static
		depends[#depends + 1] = freetype
		depends[#depends + 1] = utfz
		linux_libs[#linux_libs + 1] = "expat"
		linux_libs[#linux_libs + 1] = "pthread"
	end

	linux_libs.Config = "linux-*"

	return Program {
		Name = name,
		Includes = { "xo" },
		Libs = { linux_libs },
		Depends = depends,
		Sources = src,
	}
end

local ExampleBench       = XoExampleApp(true, "xoWinMain.cpp", "Bench", {"Bench.cpp"})
local ExampleCanvas      = XoExampleApp(true, "xoWinMain.cpp", "Canvas", {"Canvas.cpp"})
local ExampleEvents      = XoExampleApp(true, "xoWinMain.cpp", "Events", {"Events.cpp"})
local ExampleHelloWorld  = XoExampleApp(true, "xoWinMain.cpp", "HelloWorld", {"HelloWorld.cpp"})
local ExampleSplineDev   = XoExampleApp(true, "xoWinMain.cpp", "SplineDev", {"SplineDev.cpp"})
local ExampleLowLevel    = XoExampleApp(true, "xoWinMainLowLevel.cpp", "RunAppLowLevel", {"RunAppLowLevel.cpp"})
local ExampleKitchenSink = XoExampleApp(true, "xoWinMain.cpp", "KitchenSink", {"KitchenSink.cpp", "SVGSamples.cpp"})
local ExampleKitchenSink_Static = XoExampleApp(false, "xoWinMain.cpp", "KitchenSinkStatic", {"KitchenSink.cpp", "SVGSamples.cpp"})

local Test = Program {
	Name = "Test",
	--SourceDir = ".",
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
		makeGlob("tests", {}),
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

