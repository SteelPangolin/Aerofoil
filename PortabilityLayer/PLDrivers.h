#pragma once

#include "GpDriverIndex.h"

class PLDrivers
{
public:
	static GpDriverCollection *GetDriverCollection();

	static IGpAudioDriver *GetAudioDriver();
	static IGpFileSystem *GetFileSystem();
	static IGpDisplayDriver *GetDisplayDriver();
	static IGpLogDriver *GetLogDriver();
	static size_t GetNumInputDrivers();
	static IGpInputDriver *GetInputDriver(size_t index);
	static IGpSystemServices *GetSystemServices();
	static IGpFontHandler *GetFontHandler();
	static IGpVOSEventQueue *GetVOSEventQueue();

private:
	static GpDriverCollection ms_drivers;
};
