#include "GpMain.h"
#include "GpAppInterface.h"
#include "GpAudioDriverFactory.h"
#include "GpAudioDriverProperties.h"
#include "GpDisplayDriverFactory.h"
#include "GpDisplayDriverProperties.h"
#include "GpDisplayDriverTickStatus.h"
#include "GpFontHandlerFactory.h"
#include "GpFontHandlerProperties.h"
#include "GpInputDriverFactory.h"
#include "GpInputDriverProperties.h"
#include "GpGlobalConfig.h"
#include "GpAppEnvironment.h"
#include "IGpAudioDriver.h"
#include "IGpDisplayDriver.h"
#include "IGpFontHandler.h"
#include "IGpInputDriver.h"

#include <string.h>
#include <stdlib.h>

namespace
{
	void RenderAppEnvironment(void *context)
	{
		static_cast<GpAppEnvironment*>(context)->Render();
	}

	bool AdjustRequestedResolution(void *context, uint32_t &physicalWidth, uint32_t &physicalHeight, uint32_t &virtualWidth, uint32_t &virtualheight, float &pixelScaleX, float &pixelScaleY)
	{
		return static_cast<GpAppEnvironment*>(context)->AdjustRequestedResolution(physicalWidth, physicalHeight, virtualWidth, virtualheight, pixelScaleX, pixelScaleY);
	}
}

int GpMain::Run()
{
	GpVOSEventQueue *eventQueue = new GpVOSEventQueue();
	GpAppEnvironment *appEnvironment = new GpAppEnvironment();

	GpDisplayDriverProperties ddProps;
	memset(&ddProps, 0, sizeof(ddProps));

	ddProps.m_frameTimeLockNumerator = 1;
	ddProps.m_frameTimeLockDenominator = 60;

	// +/- 1% tolerance for frame time variance
	ddProps.m_frameTimeLockMinNumerator = 99;
	ddProps.m_frameTimeLockMinDenominator = 6000;
	ddProps.m_frameTimeLockMaxNumerator = 101;
	ddProps.m_frameTimeLockMaxDenominator = 6000;

	ddProps.m_renderFunc = RenderAppEnvironment;
	ddProps.m_renderFuncContext = appEnvironment;

	ddProps.m_adjustRequestedResolutionFunc = AdjustRequestedResolution;
	ddProps.m_adjustRequestedResolutionFuncContext = appEnvironment;

	ddProps.m_type = g_gpGlobalConfig.m_displayDriverType;
	ddProps.m_osGlobals = g_gpGlobalConfig.m_osGlobals;
	ddProps.m_eventQueue = eventQueue;
	ddProps.m_logger = g_gpGlobalConfig.m_logger;
	ddProps.m_systemServices = g_gpGlobalConfig.m_systemServices;

	GpAudioDriverProperties adProps;
	memset(&adProps, 0, sizeof(adProps));

	GpFontHandlerProperties fontProps;
	memset(&fontProps, 0, sizeof(fontProps));

	fontProps.m_type = g_gpGlobalConfig.m_fontHandlerType;

	// The sample rate used in all of Glider PRO's sound is 0x56ee8ba3
	// This appears to be the "standard" Mac sample rate, probably rounded from 244800/11.
	adProps.m_type = g_gpGlobalConfig.m_audioDriverType;
	adProps.m_sampleRate = (244800 * 2 + 11) / (11 * 2);
#if !GP_DEBUG_CONFIG
	adProps.m_debug = false;
#else
	adProps.m_debug = true;
#endif
	adProps.m_logger = g_gpGlobalConfig.m_logger;
	adProps.m_systemServices = g_gpGlobalConfig.m_systemServices;

	IGpInputDriver **inputDrivers = static_cast<IGpInputDriver**>(malloc(sizeof(IGpInputDriver*) * g_gpGlobalConfig.m_numInputDrivers));

	size_t numCreatedInputDrivers = 0;
	if (inputDrivers)
	{
		for (size_t i = 0; i < g_gpGlobalConfig.m_numInputDrivers; i++)
		{
			GpInputDriverProperties inputProps;
			memset(&inputProps, 0, sizeof(inputProps));

			inputProps.m_type = g_gpGlobalConfig.m_inputDriverTypes[i];
			inputProps.m_eventQueue = eventQueue;

			if (IGpInputDriver *driver = GpInputDriverFactory::CreateInputDriver(inputProps))
				inputDrivers[numCreatedInputDrivers++] = driver;
		}
	}

	IGpDisplayDriver *displayDriver = GpDisplayDriverFactory::CreateDisplayDriver(ddProps);
	IGpAudioDriver *audioDriver = GpAudioDriverFactory::CreateAudioDriver(adProps);
	IGpFontHandler *fontHandler = GpFontHandlerFactory::CreateFontHandler(fontProps);

	appEnvironment->Init();

	appEnvironment->SetDisplayDriver(displayDriver);
	appEnvironment->SetAudioDriver(audioDriver);
	appEnvironment->SetInputDrivers(inputDrivers, numCreatedInputDrivers);
	appEnvironment->SetFontHandler(fontHandler);
	appEnvironment->SetVOSEventQueue(eventQueue);
	appEnvironment->SetSystemServices(g_gpGlobalConfig.m_systemServices);

	// Start the display loop
	displayDriver->Init();

	appEnvironment->Run();

	// Clean up
	if (inputDrivers)
	{
		for (size_t i = 0; i < numCreatedInputDrivers; i++)
			inputDrivers[i]->Shutdown();

		free(inputDrivers);
	}

	return 0;
}
