#include "SDL.h"

#include "GpMain.h"
#include "GpAudioDriverFactory.h"
#include "GpDisplayDriverFactory.h"
#include "GpGlobalConfig.h"
#include "GpFileSystem_Android.h"
#include "GpFontHandlerFactory.h"
#include "GpInputDriverFactory.h"
#include "GpInputDriver_SDL_Gamepad.h"
#include "GpAppInterface.h"
#include "GpSystemServices_Android.h"
#include "GpVOSEvent.h"
#include "IGpVOSEventQueue.h"
#include "IGpLogDriver.h"

#include "IGpFileSystem.h"
#include "IGpThreadEvent.h"

#include "GpAndroid.h"

#include <exception>

GpAndroidGlobals g_gpAndroidGlobals;

IGpDisplayDriver *GpDriver_CreateDisplayDriver_SDL_GL2(const GpDisplayDriverProperties &properties);
IGpAudioDriver *GpDriver_CreateAudioDriver_SDL(const GpAudioDriverProperties &properties);

class GpLogDriver_Android final : public IGpLogDriver
{
public:
	void VPrintf(Category category, const char *fmt, va_list args) override;
	void Shutdown() override;

	static GpLogDriver_Android *GetInstance();

private:
	static GpLogDriver_Android ms_instance;
};

void GpLogDriver_Android::VPrintf(IGpLogDriver::Category category, const char *fmt, va_list args)
{
	switch (category)
	{
	case IGpLogDriver::Category_Error:
		SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, args);
		break;
	case IGpLogDriver::Category_Warning:
		SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
		break;
	case IGpLogDriver::Category_Information:
		SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
		break;
	default:
		SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE, fmt, args);
		break;
	};
}

void GpLogDriver_Android::Shutdown()
{
}

GpLogDriver_Android *GpLogDriver_Android::GetInstance()
{
	return &ms_instance;
}

GpLogDriver_Android GpLogDriver_Android::ms_instance;

int main(int argc, char* argv[])
{
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

	GpFileSystem_Android::GetInstance()->InitJNI();

	GpDriverCollection *drivers = GpAppInterface_Get()->PL_GetDriverCollection();
	drivers->SetDriver<GpDriverIDs::kFileSystem>(GpFileSystem_Android::GetInstance());
	drivers->SetDriver<GpDriverIDs::kSystemServices>(GpSystemServices_Android::GetInstance());
	drivers->SetDriver<GpDriverIDs::kLog>(GpLogDriver_Android::GetInstance());

	g_gpGlobalConfig.m_displayDriverType = EGpDisplayDriverType_SDL_GL2;

	g_gpGlobalConfig.m_audioDriverType = EGpAudioDriverType_SDL2;

	g_gpGlobalConfig.m_fontHandlerType = EGpFontHandlerType_None;

	g_gpGlobalConfig.m_inputDriverTypes = nullptr;
	g_gpGlobalConfig.m_numInputDrivers = 0;

	g_gpGlobalConfig.m_osGlobals = &g_gpAndroidGlobals;
	g_gpGlobalConfig.m_logger = GpLogDriver_Android::GetInstance();
	g_gpGlobalConfig.m_systemServices = GpSystemServices_Android::GetInstance();

	GpDisplayDriverFactory::RegisterDisplayDriverFactory(EGpDisplayDriverType_SDL_GL2, GpDriver_CreateDisplayDriver_SDL_GL2);
	GpAudioDriverFactory::RegisterAudioDriverFactory(EGpAudioDriverType_SDL2, GpDriver_CreateAudioDriver_SDL);

	int returnCode = GpMain::Run();

	GpFileSystem_Android::GetInstance()->ShutdownJNI();

	SDL_Quit();

	// This doesn't even actually exit, but it does stop this stupid brain-damaged OS from
	// just calling "main" again with no cleanup...
	// That'll have to do until proper cleanup code is added to everything.
	exit(returnCode);

	return returnCode;
}

IGpInputDriverSDLGamepad *IGpInputDriverSDLGamepad::GetInstance()
{
	return nullptr;
}
