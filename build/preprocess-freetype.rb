# This script is used by create-amalgamation.rb to produce a set of .c files
# that can be concatenated together.

NewLine = "\r\n"

# Roll xo-amalgamation-freetype-internal.h into xo-amalgamation-freetype.c.
# When producing the amalgamation, it was useful to keep these two separate, but
# for the finished product we combine them
ProduceSingleCFile = true

c_files = [
"dependencies/freetype/src/gzip/ftgzip.c",
"END-GZIP",
"dependencies/freetype/src/autofit/autofit.c",
"dependencies/freetype/src/base/ftbase.c",
"dependencies/freetype/src/base/ftbbox.c",
"dependencies/freetype/src/base/ftbitmap.c",
"dependencies/freetype/src/base/ftgasp.c",
"dependencies/freetype/src/base/ftglyph.c",
"dependencies/freetype/src/base/ftinit.c",
"dependencies/freetype/src/base/ftlcdfil.c",
"dependencies/freetype/src/base/ftmm.c",
"dependencies/freetype/src/base/ftpfr.c",
"dependencies/freetype/src/base/ftstroke.c",
"dependencies/freetype/src/base/ftsynth.c",
"dependencies/freetype/src/base/ftsystem.c",
"dependencies/freetype/src/base/fttype1.c",
"dependencies/freetype/src/base/ftwinfnt.c",
"dependencies/freetype/src/bdf/bdf.c",
"dependencies/freetype/src/cache/ftcache.c",
"dependencies/freetype/src/cff/cff.c",
"dependencies/freetype/src/cid/type1cid.c",
"dependencies/freetype/src/lzw/ftlzw.c",
"dependencies/freetype/src/pcf/pcf.c",
"dependencies/freetype/src/pfr/pfr.c",
"dependencies/freetype/src/psaux/psaux.c",
"dependencies/freetype/src/pshinter/pshinter.c",
"dependencies/freetype/src/psnames/psmodule.c",
"dependencies/freetype/src/raster/raster.c",
"dependencies/freetype/src/sfnt/sfnt.c",
"dependencies/freetype/src/smooth/smooth.c",
"dependencies/freetype/src/truetype/truetype.c",
"dependencies/freetype/src/type1/type1.c",
"dependencies/freetype/src/type42/type42.c",
"dependencies/freetype/src/winfonts/winfnt.c",
]

h_files_internal = [
"dependencies/freetype/include/freetype/internal/internal.h",
#"dependencies/freetype/include/freetype/config/ftheader.h",
#"dependencies/freetype/include/ft2build.h",
"dependencies/freetype/include/freetype/config/ftoption.h",
"dependencies/freetype/include/freetype/config/ftstdlib.h",
"dependencies/freetype/include/freetype/config/ftconfig.h",
"dependencies/freetype/include/freetype/ftsystem.h",
"dependencies/freetype/include/freetype/ftimage.h",
"dependencies/freetype/include/freetype/fttypes.h",
"dependencies/freetype/include/freetype/ftmoderr.h",
#"dependencies/freetype/include/freetype/fterrdef.h",
"dependencies/freetype/include/freetype/fterrors.h",
"dependencies/freetype/include/freetype/freetype.h",
"dependencies/freetype/include/freetype/ftmodapi.h",
"dependencies/freetype/include/freetype/ftglyph.h",
"dependencies/freetype/include/freetype/ftrender.h",
"dependencies/freetype/include/freetype/ftsizes.h",
"dependencies/freetype/include/freetype/ftlcdfil.h",
"dependencies/freetype/include/freetype/internal/ftmemory.h",
"dependencies/freetype/include/freetype/internal/ftgloadr.h",
"dependencies/freetype/include/freetype/internal/ftdriver.h",
"dependencies/freetype/include/freetype/internal/autohint.h",
"dependencies/freetype/include/freetype/internal/ftserv.h",
"dependencies/freetype/include/freetype/internal/ftpic.h",
"dependencies/freetype/include/freetype/ftincrem.h",
"dependencies/freetype/include/freetype/internal/ftobjs.h",
"dependencies/freetype/src/pshinter/pshpic.h",
"dependencies/freetype/src/pshinter/pshnterr.h",
"dependencies/freetype/include/freetype/t1tables.h",
"dependencies/freetype/include/freetype/internal/pshints.h",
"dependencies/freetype/src/autofit/aftypes.h",
"dependencies/freetype/src/autofit/afhints.h",
"dependencies/freetype/src/autofit/afloader.h",
"dependencies/freetype/src/autofit/afmodule.h",
"dependencies/freetype/src/autofit/afglobal.h",
"dependencies/freetype/src/autofit/afdummy.h",
"dependencies/freetype/src/autofit/aflatin.h",
"dependencies/freetype/src/autofit/afcjk.h",
"dependencies/freetype/src/autofit/afindic.h",
"dependencies/freetype/src/winfonts/fnterrs.h",
"dependencies/freetype/include/freetype/ftadvanc.h",
"dependencies/freetype/include/freetype/ftautoh.h",
"dependencies/freetype/include/freetype/internal/services/svprop.h",
"dependencies/freetype/src/base/basepic.h",
"dependencies/freetype/include/freetype/fttrigon.h",
"dependencies/freetype/include/freetype/ftlist.h",
"dependencies/freetype/include/freetype/internal/ftvalid.h",
"dependencies/freetype/include/freetype/internal/ftrfork.h",
"dependencies/freetype/include/freetype/internal/ftstream.h",
"dependencies/freetype/include/freetype/tttables.h",
"dependencies/freetype/include/freetype/ftwinfnt.h",
"dependencies/freetype/src/winfonts/winfnt.h",
"dependencies/freetype/include/freetype/ftmm.h",
"dependencies/freetype/include/freetype/internal/tttypes.h",
"dependencies/freetype/include/freetype/internal/sfnt.h",
"dependencies/freetype/include/freetype/tttags.h",
"dependencies/freetype/include/freetype/ttnameid.h",
"dependencies/freetype/include/freetype/internal/services/svsfnt.h",
"dependencies/freetype/include/freetype/internal/services/svpostnm.h",
"dependencies/freetype/include/freetype/internal/services/svgldict.h",
"dependencies/freetype/include/freetype/internal/services/svttcmap.h",
"dependencies/freetype/include/freetype/internal/services/svkern.h",
"dependencies/freetype/include/freetype/internal/services/svtteng.h",
"dependencies/freetype/src/base/ftbase.h",
"dependencies/freetype/include/freetype/ftsnames.h",
"dependencies/freetype/include/freetype/ftbbox.h",
"dependencies/freetype/include/freetype/ftbitmap.h",
"dependencies/freetype/include/freetype/ftgasp.h",
#"dependencies/freetype/include/freetype/config/ftmodule.h",
"dependencies/freetype/include/freetype/internal/services/svmm.h",
"dependencies/freetype/include/freetype/ftpfr.h",
"dependencies/freetype/include/freetype/internal/services/svpfr.h",
"dependencies/freetype/include/freetype/ftstroke.h",
"dependencies/freetype/include/freetype/ftsynth.h",
"dependencies/freetype/src/type42/t42drivr.h",
"dependencies/freetype/include/freetype/internal/services/svpscmap.h",
"dependencies/freetype/include/freetype/internal/t1types.h",
"dependencies/freetype/include/freetype/internal/services/svpsinfo.h",
"dependencies/freetype/include/freetype/internal/services/svwinfnt.h",
"dependencies/freetype/src/bdf/bdf.h",
"dependencies/freetype/src/bdf/bdferror.h",
"dependencies/freetype/include/freetype/ftbdf.h",
"dependencies/freetype/include/freetype/internal/services/svbdf.h",
"dependencies/freetype/include/freetype/internal/services/svxf86nm.h",
"dependencies/freetype/src/bdf/bdfdrivr.h",
"dependencies/freetype/include/freetype/ftcache.h",
"dependencies/freetype/src/cache/ftcmru.h",
"dependencies/freetype/src/cache/ftcerror.h",
"dependencies/freetype/src/cache/ftccache.h",
"dependencies/freetype/src/cache/ftcmanag.h",
"dependencies/freetype/src/cache/ftcglyph.h",
"dependencies/freetype/src/cache/ftcimage.h",
"dependencies/freetype/src/cache/ftcsbits.h",
"dependencies/freetype/src/cache/ftccback.h",
"dependencies/freetype/src/cff/cfftypes.h",
"dependencies/freetype/src/cff/cffobjs.h",
"dependencies/freetype/src/cff/cffcmap.h",
"dependencies/freetype/src/cff/cffpic.h",
"dependencies/freetype/src/cff/cfferrs.h",
"dependencies/freetype/include/freetype/internal/services/svcid.h",
"dependencies/freetype/src/cff/cffdrivr.h",
"dependencies/freetype/src/cff/cffgload.h",
"dependencies/freetype/src/cff/cffload.h",
"dependencies/freetype/src/cff/cffparse.h",
"dependencies/freetype/include/freetype/ftcffdrv.h",
#"dependencies/freetype/src/cff/cfftoken.h",
"dependencies/freetype/src/cff/cf2types.h",
"dependencies/freetype/src/cff/cf2error.h",
"dependencies/freetype/src/cff/cf2fixed.h",
"dependencies/freetype/src/cff/cf2arrst.h",
"dependencies/freetype/src/cff/cf2read.h",
"dependencies/freetype/src/cff/cf2glue.h",
"dependencies/freetype/src/cff/cf2ft.h",
"dependencies/freetype/src/cff/cf2blues.h",
"dependencies/freetype/src/cff/cf2hints.h",
"dependencies/freetype/src/cff/cf2font.h",
"dependencies/freetype/src/cff/cf2intrp.h",
"dependencies/freetype/src/cff/cf2stack.h",
"dependencies/freetype/include/freetype/internal/psaux.h",
"dependencies/freetype/src/cid/cidparse.h",
"dependencies/freetype/src/cid/ciderrs.h",
"dependencies/freetype/src/cid/cidload.h",
#"dependencies/freetype/src/cid/cidtoken.h",
"dependencies/freetype/src/cid/cidobjs.h",
"dependencies/freetype/src/cid/cidgload.h",
"dependencies/freetype/src/cid/cidriver.h",
"dependencies/freetype/include/freetype/ftgzip.h",
"dependencies/freetype/src/gzip/zconf.h",
"dependencies/freetype/src/gzip/zlib.h",
#"dependencies/freetype/src/gzip/zutil.h",
#"dependencies/freetype/src/gzip/inftrees.h",
#"dependencies/freetype/src/gzip/infblock.h",
#"dependencies/freetype/src/gzip/infcodes.h",
#"dependencies/freetype/src/gzip/infutil.h",
#"dependencies/freetype/src/gzip/inffixed.h",
"dependencies/freetype/include/freetype/ftlzw.h",
"dependencies/freetype/src/lzw/ftzopen.h",
"dependencies/freetype/src/pcf/pcfutil.h",
"dependencies/freetype/src/pcf/pcf.h",
"dependencies/freetype/src/pcf/pcfread.h",
"dependencies/freetype/src/pcf/pcferror.h",
"dependencies/freetype/include/freetype/ftbzip2.h",
"dependencies/freetype/src/pcf/pcfdrivr.h",
"dependencies/freetype/src/pfr/pfrtypes.h",
"dependencies/freetype/src/pfr/pfrobjs.h",
"dependencies/freetype/src/pfr/pfrload.h",
"dependencies/freetype/src/pfr/pfrerror.h",
"dependencies/freetype/src/pfr/pfrgload.h",
"dependencies/freetype/src/pfr/pfrsbit.h",
"dependencies/freetype/src/pfr/pfrcmap.h",
"dependencies/freetype/src/pfr/pfrdrivr.h",
"dependencies/freetype/src/psaux/psobjs.h",
"dependencies/freetype/src/psaux/psconv.h",
"dependencies/freetype/src/psaux/psauxerr.h",
"dependencies/freetype/src/psaux/psauxmod.h",
"dependencies/freetype/src/psaux/t1decode.h",
"dependencies/freetype/src/psaux/t1cmap.h",
"dependencies/freetype/src/psaux/afmparse.h",
"dependencies/freetype/src/autofit/aferrors.h",
"dependencies/freetype/src/autofit/afpic.h",
"dependencies/freetype/src/pshinter/pshglob.h",
"dependencies/freetype/src/pshinter/pshrec.h",
"dependencies/freetype/src/pshinter/pshalgo.h",
"dependencies/freetype/src/psnames/psmodule.h",
"dependencies/freetype/src/psnames/pstables.h",
"dependencies/freetype/src/psnames/psnamerr.h",
"dependencies/freetype/src/psnames/pspic.h",
"dependencies/freetype/src/type42/t42types.h",
"dependencies/freetype/src/type42/t42error.h",
"dependencies/freetype/src/type42/t42parse.h",
"dependencies/freetype/src/type42/t42objs.h",
"dependencies/freetype/src/sfnt/sfntpic.h",
"dependencies/freetype/src/sfnt/sferrors.h",
"dependencies/freetype/src/sfnt/ttload.h",
"dependencies/freetype/src/sfnt/ttmtx.h",
"dependencies/freetype/src/sfnt/ttcmap.h",
#"dependencies/freetype/src/sfnt/ttcmapc.h",
"dependencies/freetype/src/sfnt/ttkern.h",
"dependencies/freetype/src/sfnt/sfobjs.h",
"dependencies/freetype/src/sfnt/ttbdf.h",
"dependencies/freetype/src/sfnt/sfdriver.h",
"dependencies/freetype/src/sfnt/ttsbit.h",
"dependencies/freetype/src/sfnt/ttpost.h",
"dependencies/freetype/src/sfnt/pngshim.h",
"dependencies/freetype/src/smooth/ftspic.h",
#"dependencies/freetype/src/smooth/ftsmerrs.h",
"dependencies/freetype/src/smooth/ftgrays.h",
"dependencies/freetype/src/smooth/ftsmooth.h",
"dependencies/freetype/src/truetype/ttpic.h",
"dependencies/freetype/src/truetype/tterrors.h",
"dependencies/freetype/include/freetype/internal/services/svttglyf.h",
"dependencies/freetype/include/freetype/ftttdrv.h",
"dependencies/freetype/src/truetype/ttdriver.h",
"dependencies/freetype/src/truetype/ttobjs.h",
"dependencies/freetype/src/truetype/ttinterp.h",
"dependencies/freetype/src/truetype/ttgload.h",
"dependencies/freetype/src/truetype/ttpload.h",
"dependencies/freetype/src/truetype/ttgxvar.h",
"dependencies/freetype/src/truetype/ttsubpix.h",
"dependencies/freetype/src/type1/t1parse.h",
#"dependencies/freetype/src/type1/t1errors.h",
"dependencies/freetype/src/type1/t1load.h",
#"dependencies/freetype/src/type1/t1tokens.h",
"dependencies/freetype/src/type1/t1objs.h",
"dependencies/freetype/src/type1/t1gload.h",
"dependencies/freetype/src/type1/t1afm.h",
"dependencies/freetype/src/type1/t1driver.h",
"dependencies/freetype/src/raster/rastpic.h",
#"dependencies/freetype/src/raster/rasterrs.h",
"dependencies/freetype/src/raster/ftraster.h",
"dependencies/freetype/include/freetype/internal/ftdebug.h",
"dependencies/freetype/include/freetype/internal/ftcalc.h",
"dependencies/freetype/include/freetype/ftoutln.h",
"dependencies/freetype/src/raster/ftrend1.h",
]

h_files_external = %w(
dependencies/freetype/include/freetype/config/ftheader.h
dependencies/freetype/include/freetype/freetype.h
)

c_prelude = <<END
#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS 1
	#pragma warning(disable: 4146) // unary minus operator applied to unsigned type, result still unsigned
	#pragma warning(disable: 4244) // conversion from x to y, possible loss of data
	#pragma warning(disable: 4996) // CRT security
#endif
END

if !ProduceSingleCFile
	c_prelude += "include \"xo-amalgamation-freetype-internal.h\"\n"
end

h_internal_prelude = <<END
#define FT_FREETYPE_H
#define FT_BEGIN_HEADER
#define FT_END_HEADER
#define FT2_BUILD_LIBRARY
END

h_external_prelude = <<END
END

END_GZIP = <<END
#undef local
#undef exop
#undef bits
#undef MANY
END

PseudoFiles = {
	"END-GZIP" => END_GZIP,
}

IncludeMap = {
	'#include "zutil.h"' => "dependencies/freetype/src/gzip/zutil.h",
	'#include "inftrees.h"' => "dependencies/freetype/src/gzip/inftrees.h",
	'#include "infblock.h"' => "dependencies/freetype/src/gzip/infblock.h",
	'#include "infcodes.h"' => "dependencies/freetype/src/gzip/infcodes.h",
	'#include "infutil.h"' => "dependencies/freetype/src/gzip/infutil.h",
	'#include "inffixed.h"' => "dependencies/freetype/src/gzip/inffixed.h",
	'#include "cfftoken.h"' => "dependencies/freetype/src/cff/cfftoken.h",
	'#include "cidtoken.h"' => "dependencies/freetype/src/cid/cidtoken.h",
	'#include "ttcmapc.h"' => "dependencies/freetype/src/sfnt/ttcmapc.h",
	'#include "t1tokens.h"' => "dependencies/freetype/src/type1/t1tokens.h",
	'#include "rasterrs.h"' => "dependencies/freetype/src/raster/rasterrs.h",
	'#include "t1errors.h"' => "dependencies/freetype/src/type1/t1errors.h",
	'#include "ftsmerrs.h"' => "dependencies/freetype/src/smooth/ftsmerrs.h",
	'#include FT_ERRORS_H' => "dependencies/freetype/include/freetype/fterrors.h",
	'#include FT_CONFIG_MODULES_H' => "dependencies/freetype/include/freetype/config/ftmodule.h",
	'#include FT_ERROR_DEFINITIONS_H' => "dependencies/freetype/include/freetype/fterrdef.h",
	'#include FT_INTERNAL_TRACE_H' => "dependencies/freetype/include/freetype/internal/fttrace.h",
	
	# Adding these for freetype-external.h
	'#include FT_TYPES_H' => "dependencies/freetype/include/freetype/fttypes.h",
	'#include FT_CONFIG_CONFIG_H' => "dependencies/freetype/include/freetype/config/ftconfig.h",
	'#include FT_CONFIG_STANDARD_LIBRARY_H' => "dependencies/freetype/include/freetype/config/ftstdlib.h",
	'#include FT_IMAGE_H' => "dependencies/freetype/include/freetype/ftimage.h",
	'#include FT_SYSTEM_H' => "dependencies/freetype/include/freetype/ftsystem.h",

}

##################################################################################################
# Functions
##################################################################################################

def fixlines(str)
	r = ""
	str.each_line { |line|
		line.rstrip!
		line.chop! if line[-1] == "\x1A"
		r << line + NewLine
	}
	return r
end

def read_file(filename)
	return File.open(filename, "rb") { |f| f.read }
end

def process_included_c_file(filename, included_file, is_external)
	resolved_path = File.join(File.dirname(filename), included_file)
	#print("INCLUDE #{resolved_path}\n")
	return process(resolved_path, false, is_external)
end

def process(filename, is_header, is_external)
	if PseudoFiles[filename]
		return PseudoFiles[filename]
	end
	result = ""
	input = read_file(filename)
	do_output = true
	prev_line_blank = true
	input.each_line { |line|
		done = false
		IncludeMap.each { |k,v|
			if line.index(k) == 0
				result += process_included_c_file("", v, is_external)
				done = true
				break
			end
		}
		next if done

		if line.index("#include FT_MODULE_ERRORS_H") == 0
			#result += process_included_c_file("", "dependencies/freetype/include/freetype/ftmoderr.h", is_external)
			next
		end

		if line =~ /#define FT_ERR_BASE\s+FT_Mod_/
			line = "#undef FT_ERR_BASE" + NewLine + line
		end

		next if line.index("#include <ft2build.h>") == 0
		next if line =~ /#include\s+FT_/
		next if line =~ /#include\s+"[^\.]+.h"/

		if line =~ /#include\s+"([^\.]+\.c)"/
			result += process_included_c_file(filename, $1, is_external)
			next
		end

		result += line
	}
	return result
end

def concat(files, is_header, is_external)
	all = ""
	files.each { |name|
		all << process(name, is_header, is_external)
		all << NewLine
	}
	return all
end

amal_c = fixlines(c_prelude + concat(c_files, false, false))
amal_h_internal = fixlines(h_internal_prelude + concat(h_files_internal, true, false))
amal_h_external = fixlines(concat(h_files_external, true, true))
Dir.mkdir("amalgamation") if !Dir.exist?("amalgamation")
File.open("amalgamation/xo-amalgamation-freetype-pure.c", "wb") { |f| f.write(amal_c) }
File.open("amalgamation/xo-amalgamation-freetype-internal.h", "wb") { |f| f.write(amal_h_internal) }
File.open("amalgamation/xo-amalgamation-freetype.h", "wb") { |f| f.write(amal_h_external) }

if ProduceSingleCFile
	File.open("amalgamation/xo-amalgamation-freetype.c", "wb") { |f|
		f << read_file("amalgamation/xo-amalgamation-freetype-internal.h") + read_file("amalgamation/xo-amalgamation-freetype-pure.c")
	}
	File::delete("amalgamation/xo-amalgamation-freetype-internal.h")
	File::delete("amalgamation/xo-amalgamation-freetype-pure.c")
end


