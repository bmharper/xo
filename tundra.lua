
-- MSVC /analyze is very slow, so we don't want it turned on all the time,
-- which is why we keep a separate SubVariant called "analyze"

local win_linker = {
	{ "/NXCOMPAT /DYNAMICBASE";								Config = "win*" },
	{ "/DEBUG";												Config = "win*-*-debug" },
	{ "/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /RELEASE";	Config = "win*-*-release" },
}

local unix_common = {
	Env = {
		CXXOPTS = {
			{ "-std=c++11" },
		}
	}
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
		CXXOPTS = {
			{ "/analyze /WX"; Config = "win*-*-*-analyze" },
			{ "/EHsc"; Config = "win*" },
			{ "/W3"; Config = "win*" },
			{ "/wd4251"; Config = "win*" },			-- class needs to have DLL-interface...
			{ "/wd6387"; Config = "win*" },			-- 'data' could be '0':  this does not adhere to the specification for the function 'foo'
			--{ "/analyze"; Config = "win*" },
			{ "/Gm-"; Config = "win*" },
			{ "/GS"; Config = "win*" },
			{ "/RTC1"; Config = "win*-*-debug" },
			{ "/Ox"; Config = "win*-*-release" },
			{ "/arch:SSE2"; Config = "win32-*" },
		},
		CPPDEFS = {
			{ "_DEBUG";					Config = "win*-*-debug" },
			{ "NOMINMAX";				Config = "win*" },
		},
	},
}

Build {
	Units = "units.lua",
	Passes= {
		PchGen = { Name = "Precompiled Header Generation", BuildOrder = 1 },
	},
	Variants = { "debug", "release" },
	SubVariants = { "default", "analyze" },
	DefaultSubVariant = "default",
	Configs = {
		{
			Name = "macosx-gcc",
			DefaultOnHost = "macosx",
			Tools = { "gcc" },
		},
		{
			Name = "linux-gcc",
			DefaultOnHost = "linux",
			Inherit = unix_common,
			Tools = { "gcc" },
		},
		-- Other Windows configuration that you might want to enable
		--[[
		{
			Name = "win32-msvc2010",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2010"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc2010",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2010"; TargetArch = "x64"} },
		},
		{
			Name = "win32-msvc2012",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2012"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc2012",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2012"; TargetArch = "x64"} },
		},
		{
			Name = "win32-mingw",
			Tools = { "mingw" },
			-- Link with the C++ compiler to get the C++ standard library.
			ReplaceEnv = {
				LD = "$(CXX)",
			},
		},
		--]]
		{
			Name = "win32-msvc2013",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2013"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc2013",
			DefaultOnHost = "windows",
			Inherit = win_common,
			Tools = { {"msvc-vs2013"; TargetArch = "x64"} },
		},
	},
	IdeGenerationHints = {
		Msvc = {
			-- Remap config names to MSVC platform names (affects things like header scanning & debugging)
			PlatformMappings = {
				['win64-msvc2010'] = 'x64',
				['win64-msvc2012'] = 'x64',
				['win64-msvc2013'] = 'x64',
				['win32-msvc2010'] = 'Win32',
				['win32-msvc2012'] = 'Win32',
				['win32-msvc2013'] = 'Win32',
			},
			-- Remap variant names to MSVC friendly names
			VariantMappings = {
				['release-default']    = 'Release',
				['debug-default']      = 'Debug',
				['release-analyze']    = 'Release Analyze',
				['debug-analyze']      = 'Debug Analyze',
			},
		},
		-- Override solutions to generate and what units to put where.
		MsvcSolutions = {
			['nudom.sln'] = {}, -- receives all the units due to empty set
		},
		BuildAllByDefault = true,
	},
}
