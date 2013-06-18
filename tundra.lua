
local win_linker = {
	{ "/NXCOMPAT /DYNAMICBASE";		Config = "win*" },
}

local win_common = {
	Env = {
		PROGOPTS = win_linker,
		SHLIBOPTS = win_linker,
		GENERATE_PDB = {
			{ "1"; Config = "win*" },
		},
		INCREMENTAL = {
			{ "1";  Config = "win*-*-debug" },
		},
	},
}

Build {
	Units = "units.lua",
	Passes= {
		PchGen = { Name = "Precompiled Header Generation", BuildOrder = 1 },
	},
	Configs = {
		{
			Name = "macosx-gcc",
			DefaultOnHost = "macosx",
			Tools = { "gcc" },
		},
		{
			Name = "linux-gcc",
			DefaultOnHost = "linux",
			Tools = { "gcc" },
		},
		{
			Name = "win32-msvc",
			DefaultOnHost = "windows",
			Inherit = win_common,
			Tools = { {"msvc-vs2012"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc",
			DefaultOnHost = "windows",
			Inherit = win_common,
			Tools = { {"msvc-vs2012"; TargetArch = "x64"} },
		},
		--{
		--	Name = "win32-mingw",
		--	Tools = { "mingw" },
		--	-- Link with the C++ compiler to get the C++ standard library.
		--	ReplaceEnv = {
		--		LD = "$(CXX)",
		--	},
		--},
	},
}
