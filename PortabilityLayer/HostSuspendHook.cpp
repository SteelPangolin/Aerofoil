#include "HostSuspendHook.h"
#include "HostSuspendCallArgument.h"

#include "DisplayDeviceManager.h"

#include "PLDrivers.h"
#include "IGpDisplayDriver.h"


namespace PortabilityLayer
{
	void RenderFrames(unsigned int ticks)
	{
		PLDrivers::GetDisplayDriver()->ServeTicks(ticks);
		DisplayDeviceManager::GetInstance()->IncrementTickCount(ticks);
	}
}
