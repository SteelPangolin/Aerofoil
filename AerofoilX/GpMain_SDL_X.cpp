#include "SDL.h"

#include "GpMain.h"
#include "GpAudioDriverFactory.h"
#include "GpDisplayDriverFactory.h"
#include "GpGlobalConfig.h"
#include "GpFileSystem_X.h"
#include "GpLogDriver_X.h"
#include "GpFontHandlerFactory.h"
#include "GpInputDriverFactory.h"
#include "GpAppInterface.h"
#include "GpSystemServices_X.h"
#include "GpVOSEvent.h"
#include "GpX.h"

#include "IGpFileSystem.h"
#include "IGpThreadEvent.h"
#include "IGpVOSEventQueue.h"

#include <string>

GpXGlobals g_gpXGlobals;

extern "C" IGpFontHandler *GpDriver_CreateFontHandler_FreeType2(const GpFontHandlerProperties &properties);

IGpDisplayDriver *GpDriver_CreateDisplayDriver_SDL_GL2(const GpDisplayDriverProperties &properties);
IGpAudioDriver *GpDriver_CreateAudioDriver_SDL(const GpAudioDriverProperties &properties);
IGpInputDriver *GpDriver_CreateInputDriver_SDL2_Gamepad(const GpInputDriverProperties &properties);


int main(int argc, char *argv[])
{
	GpLogDriver_X::Init();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
		return -1;

	GpFileSystem_X::GetInstance()->Init();

	IGpLogDriver *logger = GpLogDriver_X::GetInstance();
	GpDriverCollection *drivers = GpAppInterface_Get()->PL_GetDriverCollection();

	drivers->SetDriver<GpDriverIDs::kFileSystem>(GpFileSystem_X::GetInstance());
	drivers->SetDriver<GpDriverIDs::kSystemServices>(GpSystemServices_X::GetInstance());
	drivers->SetDriver<GpDriverIDs::kLog>(GpLogDriver_X::GetInstance());

	g_gpGlobalConfig.m_displayDriverType = EGpDisplayDriverType_SDL_GL2;

	g_gpGlobalConfig.m_audioDriverType = EGpAudioDriverType_SDL2;

	g_gpGlobalConfig.m_fontHandlerType = EGpFontHandlerType_FreeType2;

	EGpInputDriverType inputDrivers[] =
	{
		EGpInputDriverType_SDL2_Gamepad
	};

	g_gpGlobalConfig.m_inputDriverTypes = inputDrivers;
	g_gpGlobalConfig.m_numInputDrivers = sizeof(inputDrivers) / sizeof(inputDrivers[0]);

	g_gpGlobalConfig.m_osGlobals = &g_gpXGlobals;
	g_gpGlobalConfig.m_logger = logger;
	g_gpGlobalConfig.m_systemServices = GpSystemServices_X::GetInstance();

	GpDisplayDriverFactory::RegisterDisplayDriverFactory(EGpDisplayDriverType_SDL_GL2, GpDriver_CreateDisplayDriver_SDL_GL2);
	GpAudioDriverFactory::RegisterAudioDriverFactory(EGpAudioDriverType_SDL2, GpDriver_CreateAudioDriver_SDL);
	GpInputDriverFactory::RegisterInputDriverFactory(EGpInputDriverType_SDL2_Gamepad, GpDriver_CreateInputDriver_SDL2_Gamepad);
	GpFontHandlerFactory::RegisterFontHandlerFactory(EGpFontHandlerType_FreeType2, GpDriver_CreateFontHandler_FreeType2);

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "SDL environment configured, starting up");

	int returnCode = GpMain::Run();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "SDL environment exited with code %i, cleaning up", returnCode);

	return returnCode;
}
