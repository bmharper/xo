# This script is used to build up a sorted list of all header files that are included by all the .c/.cpp files listed here
# It should be easy to adapt this script to analyze a different set of .c/.cpp files.

# Only output files matching this extension
include_filter = /.h$/

cl_options = "/Idependencies/freetype/include /DFT2_BUILD_LIBRARY"

c_files = %w(
dependencies/freetype/src/autofit/autofit.c
dependencies/freetype/src/base/ftbase.c
dependencies/freetype/src/base/ftbbox.c
dependencies/freetype/src/base/ftbitmap.c
dependencies/freetype/src/base/ftgasp.c
dependencies/freetype/src/base/ftglyph.c
dependencies/freetype/src/base/ftinit.c
dependencies/freetype/src/base/ftlcdfil.c
dependencies/freetype/src/base/ftmm.c
dependencies/freetype/src/base/ftpfr.c
dependencies/freetype/src/base/ftstroke.c
dependencies/freetype/src/base/ftsynth.c
dependencies/freetype/src/base/ftsystem.c
dependencies/freetype/src/base/fttype1.c
dependencies/freetype/src/base/ftwinfnt.c
dependencies/freetype/src/bdf/bdf.c
dependencies/freetype/src/cache/ftcache.c
dependencies/freetype/src/cff/cff.c
dependencies/freetype/src/cid/type1cid.c
dependencies/freetype/src/gzip/ftgzip.c
dependencies/freetype/src/lzw/ftlzw.c
dependencies/freetype/src/pcf/pcf.c
dependencies/freetype/src/pfr/pfr.c
dependencies/freetype/src/psaux/psaux.c
dependencies/freetype/src/pshinter/pshinter.c
dependencies/freetype/src/psnames/psmodule.c
dependencies/freetype/src/raster/raster.c
dependencies/freetype/src/sfnt/sfnt.c
dependencies/freetype/src/smooth/smooth.c
dependencies/freetype/src/truetype/truetype.c
dependencies/freetype/src/type1/type1.c
dependencies/freetype/src/type42/type42.c
dependencies/freetype/src/winfonts/winfnt.c
)

# Ensure that all paths are relative to the current directory, if they are rooted inside it.
def normalize(path)
	fixed = path.gsub("\\", "/")
	fixed = fixed[Dir.pwd.length + 1 .. -1] if fixed.downcase.index(Dir.pwd.downcase) == 0
	#print("#{path} -> (#{fixed})\n")
	return fixed
end

# produce a single sorted list, from a set of sorted partially-populated lists
def unified_sort(lists)
	# build the set of unique items
	all_set = {}
	lists.each { |list|
		list.each { |file|
			all_set[file] = true
		}
	}
	all = all_set.keys

	# sort 'all', with a special sort function that looks across the entire spectrum of all lists
	# If two files always appear in the same order, in all of the lists, then their relative sort order
	# is clear. However, if the files appear in different orders in different lists, then they are
	# problematic, and need more attention.

	# keys are file names, values are number of inconsistencies
	bad = {}

	all.sort! { |x,y|
		x_less = 0
		y_less = 0
		lists.each { |list|
			x_index = list.index(x)
			y_index = list.index(y)
			if x_index != nil && y_index != nil
				x_less += 1 if x_index < y_index
				y_less += 1 if y_index < x_index
			end
		}
		if x_less > 0 && y_less > 0
			bad[x] = bad[x] || 0
			bad[y] = bad[y] || 0
			bad[x] += 1
			bad[y] += 1
		end
		if x_less > y_less
			-1
		elsif x_less == y_less
			0
		else
			1
		end
	}

	return all, bad
end

# build up a sort order for each file in the listing.
# It's not as simple as the order in which the files were
# included, because one has to pay special attention to the
# include hierarchy. For example, if windows.h includes math.h,
# then math.h must sort before windows.h, despite the fact that
# math.h would appear after windows.h in the original list.
# The /showIncludes output includes the hierarchy, so here we
# are using that information to produce a global ordering.
def sort_depth(list)
	max_depth = 0
	list.each { |pair|
		max_depth = [max_depth, pair[:depth]].max
	}
	count = 100
	ndigits = 2
	while list.length >= count do
		count *= 10
		ndigits += 1
	end
	format = "%0#{ndigits}d_"
	midpoint = count / 2
	seen = {}
	all = []
	count = [midpoint] * (max_depth + 1)
	current_depth = 0
	list.each { |pair|
		next if seen[pair[:file]]
		if pair[:depth] > current_depth
			#print("#{pair[:file]} is deeper\n")
			count[pair[:depth]] = 0
		elsif pair[:depth] < current_depth
			for i in pair[:depth]+1 .. current_depth do
				count[i] = midpoint
			end
		end
		current_depth = pair[:depth]
		count[current_depth] += 1
		order = ""
		count.each { |c|
			order += format % c
		}
		order.chop!
		all << {:order => order, :depth => pair[:depth], :file => pair[:file]}
		seen[pair[:file]] = true
	}
	return all
end

all_lists = []

c_files.each { |cfile|
	list = []
	output = `cl /nologo /c /showIncludes #{cl_options} #{cfile}`
	output.each_line { |line|
		if line =~ /Note: including file: (\s*)(.+)/
			depth = $1.length
			included = normalize($2)
			#next if included !~ include_filter
			list << {:depth => depth, :file => included}
			#print("#{included}\n")
		end
		if line =~ /fatal error/
			print(line)
		end
	}
	augmented = sort_depth(list)
	#augmented.each { |pair|
	#	depth_str = " " * pair[:depth]
	#	print("%s %s%s\n" % [pair[:order], depth_str, pair[:file]])
	#}
	sorted = augmented.sort { |a,b|
		a[:order] <=> b[:order]
	}
	sorted.select! { |x| x[:file] =~ include_filter }
	all_lists << sorted.map { |x| x[:file] }
	#print("Output: (#{output})\n")
	#break
}

print("----------------------------------------------------------------\n")
unified, bad = unified_sort(all_lists)
unified.each { |file|
	print("%3d %s\n" % [bad[file] || 0, file])
}
