setlocal
set ide=msvc120
if '%1'=='' goto default
	set ide = %1
	shift
:default
tundra2 -g %ide% %*