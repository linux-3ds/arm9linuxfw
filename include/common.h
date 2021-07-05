#pragma once

#define BIT(x)      (1ULL << (x))

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Extra type definitions / aliases */
typedef unsigned int uint;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef volatile u8  vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef volatile s8  vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;


/* C language convenience macros */
#define asmv __asm__ volatile

#define COUNT_OF(x)	(sizeof(x) / sizeof(*(x)))

#define DBG_ASSERT(x)	\
	do { \
		if (UNLIKELY(!(x))) { \
			__builtin_trap(); \
			__builtin_unreachable(); \
		} \
	}while(0)

#define CONTAINER_OF(p, t, m) \
	((t*)((const char*)(p) - offsetof(t, m)))

#define TOP_BIT(n)	((n) ? (31 - __builtin_clz(n)) : -1)
#define LOW_BIT(n)	((n) ? __builtin_ctz(n) : -1)

#define ALIGN_DOWN(n, a)	(((n) / (a)) * (a))
#define ALIGN_UP(n, a)	ALIGN_DOWN((n) + (a) - 1, (a))

#define INT_IS_POW2(x)	(((x) & ((x)-1)) == 0)
#define MOD_POW2(x, y)	((x) & ((y) - 1))

#define MIN(x, y)	((x)<(y) ? (x):(y)) // conditional execution
#define MAX(x, y)	((x)>(y) ? (x):(y)) // makes these ops fast


/* Compiler annotations */

#define FN_HOT	__attribute__((hot))
#define FN_ARM	__attribute__((target("arm")))
#define FN_THUMB	__attribute__((target("thumb")))
#define FN_NOINLINE	__attribute__((noinline))
#define FN_RETNONNULL	__attribute__((returns_nonnull))
#define FN_MUSTCHECK	__attribute__((warn_unused_result))

#define ATTR_USED	__attribute__((used))
#define APACKED(x)	__attribute__((packed, aligned(x)))
#define PACKED	APACKED(4) /* defaults to 4 byte alignment */

#define NORETURN	__attribute__((noreturn))
#define ALIGNED(n)	__attribute__((aligned(n)))

#define LIKELY(x)	__builtin_expect(!!(x), 1)
#define UNLIKELY(x)	__builtin_expect(!!(x), 0)

#define SECTION(s)	__attribute__((section(s)))

#define KEEP	__attribute__((used))
#define UNUSED	__attribute__((unused))

#endif // __ASSEMBLER__
