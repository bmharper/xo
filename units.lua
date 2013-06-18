
local nudom = SharedLibrary {
	Name = "nudom",
	Libs = { "opengl32.lib", "user32.lib", "gdi32.lib" },
	SourceDir = "nudom/",
	Includes = { "nudom/" },
	PrecompiledHeader = {
		Source = "nudom/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		"nuDefs.cpp",
		"nuDoc.cpp",
		"nuDomEl.cpp",
		"nuEvent.cpp",
		"nuLayout.cpp",
		"nuMem.cpp",
		"nuQueue.cpp",
		"nuPlatform.cpp",
		"nuString.cpp",
		"nuStringTable.cpp",
		"nuStyle.cpp",
		"nuStyleParser.cpp",
		"nuSysWnd.cpp",
		"nuProcessor.cpp",
		"nuProcessor_Win32.cpp",
		"Render/nuRenderer.cpp",
		"Render/nuRenderGL.cpp",
		"Render/nuRenderDoc.cpp",
		"Render/nuRenderDomEl.cpp",
		"Render/nuStyleResolve.cpp",
		"Text/nuTextCache.cpp",
		"Text/nuTextDefs.cpp",
		"../dependencies/Panacea/Containers/queue.cpp",
		"../dependencies/Panacea/Platform/syncprims.cpp",
		"../dependencies/Panacea/Platform/err.cpp",
		"../dependencies/Panacea/Strings/fmt.cpp",
		"../dependencies/glext.cpp",
	},
}

local HelloWorld = Program {
	Name = "HelloWorld",
	Depends = {
		nudom
	},
	Sources = {
		"nudom/nuWinMain.cpp",
		"samples/HelloWorld/HelloWorld.cpp",
	}
}

local Bench = Program {
	Name = "Bench",
	Depends = {
		nudom
	},
	Sources = {
		"nudom/nuWinMain.cpp",
		"samples/Bench/Bench.cpp",
	}
}

Default(nudom)
Default(HelloWorld)
Default(Bench)
