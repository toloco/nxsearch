/*
 * Copyright (c) 2020 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

/*
 * A regular assert (debug/diagnostic only).
 */

#if defined(DEBUG)
#define	ASSERT		assert
#else
#define	ASSERT(x)
#endif

/*
 * Branch prediction macros.
 */

#ifndef __predict_true
#define	__predict_true(x)	__builtin_expect((x) != 0, 1)
#define	__predict_false(x)	__builtin_expect((x) != 0, 0)
#endif

/*
 * Various C helpers and attribute macros.
 */

#ifndef __packed
#define	__packed		__attribute__((__packed__))
#endif

#ifndef __aligned
#define	__aligned(x)		__attribute__((__aligned__(x)))
#endif

#ifndef __unused
#define	__unused		__attribute__((__unused__))
#endif

#ifndef __arraycount
#define	__arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
#endif

#ifndef __UNCONST
#define	__UNCONST(a)		((void *)(unsigned long)(const void *)(a))
#endif

#define	__align32		__attribute__((aligned(__alignof__(uint32_t))))
#define	__align64		__attribute__((aligned(__alignof__(uint64_t))))

/*
 * Minimum, maximum and rounding macros.
 */

#ifndef MIN
#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#endif

#ifndef roundup2
#define	roundup2(x,m)	((((x) - 1) | ((m) - 1)) + 1)
#endif

/*
 * Atomics.
 */

#define	atomic_load_acquire(p)		\
    atomic_load_explicit((p), memory_order_acquire)

#define	atomic_store_release(p, v)	\
    atomic_store_explicit((p), (v), memory_order_release)

/*
 * Byte-order conversions.
 */
#if defined(__linux__) || defined(sun)
#include <endian.h>
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define	be16toh(x)	ntohs(x)
#define	htobe16(x)	htons(x)
#define	be32toh(x)	ntohl(x)
#define	htobe32(x)	htonl(x)
#define	be64toh(x)	OSSwapBigToHostInt64(x)
#define	htobe64(x)	OSSwapHostToBigInt64(x)
#else
#include <sys/endian.h>
#endif

/*
 * Misc interfaces.
 */

#endif
