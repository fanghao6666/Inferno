#pragma once

#define HAS_BIT_FLAG(x, flag) ((x & flag) == flag)
#define BIT_FLAG(x) (1 << x)
#define SET_BIT(number, n) (number |= (1 << n))
#define CLEAR_BIT(number, n) (number &= ~(1 << n))
#define BIT_MASK(n) ((1 << n) - 1)

#define BIT_FLAG_64(x) (1ull << x)
#define SET_BIT_64(number, n) (number |= (1ull << n))
#define CLEAR_BIT_64(number, n) (number &= ~(1ull << n))
#define BIT_MASK_64(n) ((1ull << n) - 1ull)

#define WRITE_BIT_RANGE_64(value, dst, offset, num_bits) (dst |= (static_cast<uint64_t>(value & BIT_MASK(num_bits)) << offset))
#define READ_BIT_RANGE_64(src, offset, num_bits) ((src >> offset) & BIT_MASK(num_bits))

#if defined(_MSC_VER)
#    define INFERNO_ALIGNED(x) __declspec(align(x))
#else
#    if defined(__GNUC__) || defined(__clang__)
#        define INFERNO_ALIGNED(x) __attribute__((aligned(x)))
#    endif
#endif

#define INFERNO_ZERO_MEMORY(x) memset(&x, 0, sizeof(x))

#define INFERNO_SAFE_DELETE(x) \
    if (x)                    \
    {                         \
        delete x;             \
        x = nullptr;          \
    }
#define INFERNO_SAFE_DELETE_ARRAY(x) \
    if (x)                          \
    {                               \
        delete[] x;                 \
        x = nullptr;                \
    }