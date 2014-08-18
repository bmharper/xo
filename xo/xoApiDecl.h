#pragma once

#ifdef _WIN32
	#ifndef NUAPI
		#define NUAPI __declspec(dllexport)
	#else
		#define NUAPI __declspec(dllimport)
	#endif
#else
	#define NUAPI
#endif
