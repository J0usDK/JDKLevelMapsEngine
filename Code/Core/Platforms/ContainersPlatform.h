#pragma once
#include <CryCore/BaseTypes.h>

#if defined(_MSC_VER)
	#include <intrin.h>
#endif

#if !defined(_RELEASE)
	#define DBG_PROBE_INIT() uint32 dbgProbes = 0
	#define DBG_PROBE_CHECK(maxSize) \
				CRY_ASSERT_MESSAGE(++dbgProbes <= (maxSize), "[JDKLevelMaps] Infinite loop detected in OpenAddressTable")
#else
	#define DBG_PROBE_INIT()
	#define DBG_PROBE_CHECK(maxSize) ((void)0)
#endif

#if defined(__GNUC__) || defined(__clang__)
	#define COMPUTE_NEXT_POT_RETURN(v) return 1ULL << (32 - __builtin_clz(v))
#elif defined(_MSC_VER)
	#define COMPUTE_NEXT_POT_RETURN(v) \
			unsigned long idx; \
			_BitScanReverse(&idx, v); \
			return 1ULL << (idx + 1)
#else
	#define COMPUTE_NEXT_POT_RETURN(v) \
		v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; \
		return ++v
#endif