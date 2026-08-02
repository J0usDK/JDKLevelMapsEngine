#pragma once

#if !defined(_RELEASE)
	#define JDK_ASSERT_FATAL(cond, msg) \
		CRY_ASSERT_MESSAGE(cond, "[JDKLevelMaps] [FATAL] " msg)

	#define JDK_ASSERT_ERROR(cond, msg) \
		CRY_ASSERT_MESSAGE(cond, "[JDKLevelMaps] [ERROR] " msg)

	#define JDK_SOFT_ASSERT(cond, msg) \
		do { \
			if (!(cond)) { \
				CryLogAlways("$4[JDKLevelMaps] [ERROR] " msg); \
			} \
		} while (false)

	#define JDK_PERF_WARN(msg) \
		CryLogAlways("$6[JDKLevelMaps] [PERF] " msg)
#else
	#define JDK_ASSERT_FATAL(cond, msg) ((void)0)
	#define JDK_ASSERT_ERROR(cond, msg) ((void)0)
	#define JDK_SOFT_ASSERT(cond, msg)	((void)0)
	#define JDK_PERF_WARN(msg)			((void)0)
#endif