rem set runmode=--dry-run
set runmode=
astyle --options=build\.astylerc --recursive --suffix=none %runmode% dependencies\Panacea\*.cpp dependencies\Panacea\*.h
astyle --options=build\.astylerc --recursive --suffix=none --exclude=StackWalker.cpp --exclude=StackWalker.h %runmode% dependencies\TinyTest\*.cpp dependencies\TinyTest\*.h
astyle --options=build\.astylerc --recursive --suffix=none %runmode% xo\*.cpp xo\*.h
astyle --options=build\.astylerc --recursive --suffix=none %runmode% tests\*.cpp tests\*.h
astyle --options=build\.astylerc --recursive --suffix=none %runmode% examples\*.cpp
astyle --options=build\.astylerc --recursive --suffix=none %runmode% templates\*.cpp
