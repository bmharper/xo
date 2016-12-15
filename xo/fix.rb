require 'fileutils'

Dir.glob("**/*.cpp").each { |f|
	`c:/dev/tools/clang-format -i -style=file #{f}`
}

Dir.glob("**/*.h").each { |f|
	`c:/dev/tools/clang-format -i -style=file #{f}`
}

=begin
Dir.glob("**/xo*.h").each { |f|
	dir = File.dirname(f)
	dir = "" if dir == "."
	file = File.basename(f)
	file = file[2..-1]
	joined = dir == "" ? file : "#{dir}/#{file}"
	FileUtils.mv(f, joined)
	print("#{dir}/#{file}\n")
	#all = File.read(f)
	#lines = ['#pragma once' + "\n", 'namespace xo {' + "\n"]
	#all.lines { |line|
	#	lines << line if line.index('#pragma once').nil?
	#}
	#lines += ['}' + "\n"]
	#all = lines.join("")
	#File.write(f, all)
}


Dir.glob("**/xo*.cpp").each { |f|
	dir = File.dirname(f)
	dir = "" if dir == "."
	file = File.basename(f)
	file = file[2..-1]
	joined = dir == "" ? file : "#{dir}/#{file}"
	FileUtils.mv(f, joined)
	print("#{dir}/#{file}\n")
	#all = File.read(f)
	#lines = ['namespace xo {' + "\n"]
	#all.lines { |line|
	#	lines << line
	#}
	#lines += ['}' + "\n"]
	#all = lines.join("")
	#File.write(f, all)
}
=end