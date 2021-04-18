#pragma once

#include <stdint.h>
#include <stddef.h>

#if __cplusplus >= 199711L
#define GP_IS_CPP11 1
#else
#define GP_IS_CPP11 0
#endif

#ifdef _MSC_VER
#define GP_ALIGNED(n) __declspec(align(n))
#else
#define GP_ALIGNED(n) __attribute__((aligned(n)))
#endif

#if GP_IS_CPP11
#define GP_DELETED = delete
#define GP_STATIC_ASSERT(n) static_assert((n), "Static assert failed: " #n)
#else
#ifndef nullptr
#define nullptr 0
#endif

#ifndef override
#define override
#endif

#ifndef final
#define final
#endif

#define GP_DELETED

template<bool TCondition>
struct __GpStaticAssertHelper
{
};

template<>
struct __GpStaticAssertHelper<true>
{
	int staticAssertFailed;
};

#define GP_STATIC_ASSERT(n) ((void)(&static_cast<const __GpStaticAssertHelper<(n)>*>(nullptr)->staticAssertFailed))

#endif

static const size_t GP_SYSTEM_MEMORY_ALIGNMENT = 16;

#if !defined(GP_DEBUG_CONFIG)
#error "GP_DEBUG_CONFIG was not set"
#endif

#ifdef __EMSCRIPTEN__
#define GP_ASYNCIFY_PARANOID	1
#else
#define GP_ASYNCIFY_PARANOID	0
#endif

#if GP_ASYNCIFY_PARANOID
#define GP_ASYNCIFY_PARANOID_VIRTUAL
#define GP_ASYNCIFY_PARANOID_PURE
#define GP_ASYNCIFY_PARANOID_OVERRIDE
#else
#define GP_ASYNCIFY_PARANOID_VIRTUAL virtual
#define GP_ASYNCIFY_PARANOID_PURE = 0
#define GP_ASYNCIFY_PARANOID_OVERRIDE override
#endif
