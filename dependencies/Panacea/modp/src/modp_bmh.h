/*
#if !defined(MODP_API)
	#if defined(PAPI)
		#define MODP_API PAPI
	#else
		#define MODP_API
	#endif
#endif
*/

/*

BMH:
Changes that I made:

* I did the whole MODP_API thing.
* I added pragma warning push/pop
* This list used to be larger, until I upgraded to the latest modp, which fixed all of the issues that I addressed locally.

*/
