#ifndef _TOOLS_LINUX_COMPILER_H_
#define _TOOLS_LINUX_COMPILER_H_

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#ifndef barrier
#define barrier() __asm__ __volatile__("": : :"memory")
#endif

//asm volatile ("" : : : "memory");

#ifndef smp_mb
#define smp_mb()	barrier()
#endif

#ifndef smp_rmb
#define smp_rmb()	smp_mb()
#endif

#ifndef smp_wmb
#define smp_wmb()	smp_mb()
#endif

#ifndef __always_inline
# define __always_inline	inline __attribute__((__always_inline__))
#endif

#define __user

#ifndef __attribute_const__
# define __attribute_const__
#endif

#ifndef __maybe_unused
# define __maybe_unused		__attribute__((unused))
#endif

#ifndef __packed
# define __packed		__attribute__((__packed__))
#endif

#ifndef __force
# define __force
#endif

#ifndef __weak
# define __weak			__attribute__((weak))
#endif

#ifndef likely
#define likely(x)  __builtin_expect((x),1)
#endif /* likely */

#ifndef unlikely
#define unlikely(x)  __builtin_expect((x),0)
#endif /* unlikely */


#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#include <linux/types.h>

/*
 * Following functions are taken from kernel sources and
 * break aliasing rules in their original form.
 *
 * While kernel is compiled with -fno-strict-aliasing,
 * perf uses -Wstrict-aliasing=3 which makes build fail
 * under gcc 4.4.
 *
 * Using extra __may_alias__ type to allow aliasing
 * in this case.
 */
typedef __u8  __attribute__((__may_alias__))  __u8_alias_t;
typedef __u16 __attribute__((__may_alias__)) __u16_alias_t;
typedef __u32 __attribute__((__may_alias__)) __u32_alias_t;
typedef __u64 __attribute__((__may_alias__)) __u64_alias_t;

static __always_inline void __read_once_size(const volatile void *p, void *res, int size)
{
    switch (size) {
    case 1: *(__u8_alias_t  *) res = *(volatile __u8_alias_t  *) p; break;
    case 2: *(__u16_alias_t *) res = *(volatile __u16_alias_t *) p; break;
    case 4: *(__u32_alias_t *) res = *(volatile __u32_alias_t *) p; break;
    case 8: *(__u64_alias_t *) res = *(volatile __u64_alias_t *) p; break;
    default:
        barrier();
        __builtin_memcpy((void *)res, (const void *)p, size);
        barrier();
    }
}

static __always_inline void __write_once_size(volatile void *p, void *res, int size)
{
    switch (size) {
    case 1: *(volatile  __u8_alias_t *) p = *(__u8_alias_t  *) res; break;
    case 2: *(volatile __u16_alias_t *) p = *(__u16_alias_t *) res; break;
    case 4: *(volatile __u32_alias_t *) p = *(__u32_alias_t *) res; break;
    case 8: *(volatile __u64_alias_t *) p = *(__u64_alias_t *) res; break;
    default:
        barrier();
        __builtin_memcpy((void *)p, (const void *)res, size);
        barrier();
    }
}


#endif /* _TOOLS_LINUX_COMPILER_H */
