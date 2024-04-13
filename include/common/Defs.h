#pragma once
#include <cstdlib>
#include <cstdint>

namespace lu::common
{
    #if defined(__GNUC__) || defined(__clang__)
        #define LIKELY(expr) __builtin_expect(static_cast<bool>(expr), 1)
        #define UNLIKELY(expr) __builtin_expect(static_cast<bool>(expr), 0)
        #define NOINLINE __attribute__((noinline))
    #else
        #define LIKELY(expr) (expr)
        #define UNLIKELY(expr) (expr)
        #define NOINLINE
    #endif

    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <xmmintrin.h>
    constexpr int CACHE_LINE_SIZE = 64;
    static inline void spinLoopPause() noexcept 
    {
        _mm_pause();
    }

    #elif defined(__arm__) || defined(__aarch64__)
    constexpr int CACHE_LINE_SIZE = 64;
    static inline void spinLoopPause() noexcept 
    {
        #if (defined(__ARM_ARCH_6K__) || \
            defined(__ARM_ARCH_6Z__) || \
            defined(__ARM_ARCH_6ZK__) || \
            defined(__ARM_ARCH_6T2__) || \
            defined(__ARM_ARCH_7__) || \
            defined(__ARM_ARCH_7A__) || \
            defined(__ARM_ARCH_7R__) || \
            defined(__ARM_ARCH_7M__) || \
            defined(__ARM_ARCH_7S__) || \
            defined(__ARM_ARCH_8A__) || \
            defined(__aarch64__))
            asm volatile ("yield" ::: "memory");
        #else
            asm volatile ("nop" ::: "memory");
        #endif
    }
    
    #elif defined(__ppc64__) || defined(__powerpc64__)
    constexpr int CACHE_LINE_SIZE = 128; // TODO: Review that this is the correct value.
    static inline void spinLoopPause() noexcept 
    {
        asm volatile("or 31,31,31 # very low priority"); // TODO: Review and benchmark that this is the right instruction.
    }

    #elif defined(__s390x__)
    constexpr int CACHE_LINE_SIZE = 256; // TODO: Review that this is the correct value.
    static inline void spinLoopPause() noexcept {} // TODO: Find the right instruction to use here, if any.

    #elif defined(__riscv)
    constexpr int CACHE_LINE_SIZE = 64;
    static inline void spinLoopPause() noexcept 
    {
        asm volatile (".insn i 0x0F, 0, x0, x0, 0x010");
    }

    #else
    #warning "Unknown CPU architecture. Using L1 cache line size of 64 bytes and no spinloop pause instruction."
    constexpr int CACHE_LINE_SIZE = 64; // TODO: 
    static inline void spinLoopPause() noexcept {}
    #endif

    template<class T>
    constexpr T nil() noexcept 
    {
    #if __cpp_lib_atomic_is_always_lock_free
        static_assert(std::atomic<T>::is_always_lock_free, "Element type T is not atomic.");
    #endif
        return {};
    }

    constexpr bool isLittleEndian()
    {
        // Use a union to interpret the bytes of an integer
        union
        {
            uint32_t i;
            char c[4];
        } endianCheck = {0x01020304};

        // If the first byte is 1, it's little-endian
        return (endianCheck.c[0] == 1);
    }
}