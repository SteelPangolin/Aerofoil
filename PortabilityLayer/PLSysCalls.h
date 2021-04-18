#pragma once

#include "CoreDefs.h"
#include "VirtualDirectory.h"

#include <stdint.h>

namespace PLSysCalls
{
	void Sleep(uint32_t ticks);
	void Exit(int exitCode);

#if GP_DEBUG_CONFIG && GP_ASYNCIFY_PARANOID
	class AsyncifyDisarmScope
	{
	public:
		AsyncifyDisarmScope();
		~AsyncifyDisarmScope();
	};

#define PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE()	PLSysCalls::AsyncifyDisarmScope disarmScope
#else
#define PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE()
#endif

	int MainExitWrapper(int (*mainFunc)());
}
