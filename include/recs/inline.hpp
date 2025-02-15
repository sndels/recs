#pragma once

#if defined(_MSC_VER)

#define RECS_FORCEINLINE __forceinline

#elif defined(__clang__) || defined(__GNUC__)

#define RECS_FORCEINLINE __attribute__((always_inline)) inline

#else

#error "RECS_FORCEINLINE not implemented for this compiler"

#endif
