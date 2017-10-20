// clang-format off
#pragma once

// Source: http://www.isthe.com/chongo/src/fnv/hash_32a.c

/*
 * 32 bit magic FNV-1a prime
 */
#define FNV_32_PRIME ((uint32_t) 0x01000193)

#define FNV1_32_INIT ((uint32_t) 0x811c9dc5)
#define FNV1_32A_INIT FNV1_32_INIT

/*
 * fnv_32a_buf - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the recommended 32 bit FNV-1a hash, use FNV1_32A_INIT as the
 * 	 hval arg on the first call to either fnv_32a_buf() or fnv_32a_str().
 */
inline uint32_t fnv_32a_buf(const void *buf, size_t len, uint32_t hval)
{
	const unsigned char *bp = (const unsigned char *) buf; /* start of buffer */
	const unsigned char *be = bp + len;              /* beyond end of buffer */

	/*
     * FNV-1a hash each octet in the buffer
     */
	while (bp < be)
	{
		/* xor the bottom with the current octet */
		hval ^= (uint32_t) *bp++;

/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif
	}

	/* return our new hash value */
	return hval;
}

inline uint32_t fnv_32a_buf(const void *buf, size_t len)
{
	return fnv_32a_buf(buf, len, FNV1_32A_INIT);
}

/*
 * fnv_32a_str - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a string
 *
 * input:
 *	str	- string to hash
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the recommended 32 bit FNV-1a hash, use FNV1_32A_INIT as the
 *  	 hval arg on the first call to either fnv_32a_buf() or fnv_32a_str().
 */
inline uint32_t fnv_32a_str(const char *str, uint32_t hval)
{
	const unsigned char *s = (const unsigned char *) str; /* unsigned string */

	/*
     * FNV-1a hash each octet in the buffer
     */
	while (*s)
	{
		/* xor the bottom with the current octet */
		hval ^= (uint32_t) *s++;

/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif
	}

	/* return our new hash value */
	return hval;
}

inline uint32_t fnv_32a_str(const char *str)
{
	return fnv_32a_str(str, FNV1_32A_INIT);
}
