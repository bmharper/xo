require 'fileutils'

Dir.glob("xo/**/*.cpp").each { |f|
	`c:/dev/tools/clang-format -i -style=file #{f}`
}

Dir.glob("xo/**/*.h").each { |f|
	`c:/dev/tools/clang-format -i -style=file #{f}`
}
