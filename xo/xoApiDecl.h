#pragma once

#ifdef _WIN32
	#ifndef XOAPI
		#define XOAPI __declspec(dllexport)
	#else
		#define XOAPI __declspec(dllimport)
	#endif
#else
	#define XOAPI
#endif
