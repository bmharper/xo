
-- MSVC /analyze is very slow, so we don't want it turned on all the time,
-- which is why we keep a separate SubVariant called "analyze"

-- How to build and test with afl?
-- Uncomment the afl Config down below, in the Configs section
-- export LD_LIBRARY_PATH=`pwd`/t2-output/linux-afl-release-default
-- ../afl-2.51b/afl-fuzz -m 200 -i testdata/fuzz-vdom-diff -o findings -- t2-output/linux-afl-release-default/Test fuzz-vdom-diff

local win_linker = {
	{ "/NXCOMPAT /DYNAMICBASE /DEBUG:FASTLINK";				Config = "win*" },
	{ "/DEBUG";												Config = "win*-*-debug" },
	{ "/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /RELEASE";	Config = "win*-*-release" },
}

local unix_common = {
	Env = {
		CXXOPTS = {
			{ "-std=c++11" },
			{ "-fPIC" },
			{ "-ggdb" },
		},
		CCOPTS = {
			{ "-fPIC" },
			{ "-ggdb" },
		},
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
			{ "/Gm-"; Config = "win*" },
			{ "/GS"; Config = "win*" },
			{ "/RTC1"; Config = "win*-*-debug" },
			{ "/Ox"; Config = "win*-*-release" },
			{ "/arch:SSE2"; Config = "win32-*" },
			{ "/openmp"; Config = "win*" },
			-- { "/Zc:inline"; Config = "win*" },		-- This requires VC 2013 Update 2, but it's really just a compiler/linker performance improvement.
		},
		CPPDEFS = {
			{ "_DEBUG";					Config = "win*-*-debug" },
			{ "NOMINMAX";				Config = "win*" },
		},
	},
}

Build {
	ScriptDirs = { "build" }, -- Allow tundra to find our afl toolchain inside build/tundra/tools/afl.lua
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
			Name = "linux-clang",
			DefaultOnHost = "linux",
			Inherit = unix_common,
			Tools = { "clang" },
		},
		-- See instructions at top of file for using afl (American Fuzzy Lop fuzzer).
		{
			Name = "linux-afl",
			SupportedHosts = { "linux" },
			Inherit = unix_common,
			Tools = { "afl" },
		},
		{
			Name = "win32-msvc2015",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2015"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc2015",
			DefaultOnHost = "windows",
			Inherit = win_common,
			Tools = { {"msvc-vs2015"; TargetArch = "x64"} },
		},
	},
	IdeGenerationHints = {
		Msvc = {
			-- Remap config names to MSVC platform names (affects things like header scanning & debugging)
			PlatformMappings = {
				['win64-msvc2015'] = 'x64',
				['win32-msvc2015'] = 'Win32',
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
			['xo.sln'] = {}, -- receives all the units due to empty set
		},
		-- Override output directory for sln/vcxproj files.
		MsvcSolutionDir = 'ide',
		BuildAllByDefault = true,
	},
}
