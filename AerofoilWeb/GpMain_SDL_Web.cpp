#include <SDL.h>
#include "SDL_main.h"

#include "GpMain.h"
#include "GpAudioDriverFactory.h"
#include "GpDisplayDriverFactory.h"
#include "GpGlobalConfig.h"
#include "GpFileSystem_Web.h"
#include "GpLogDriver_Web.h"
#include "GpFontHandlerFactory.h"
#include "GpInputDriverFactory.h"
#include "GpAppInterface.h"
#include "GpSystemServices_Web.h"
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


int main(int argc, char* argv[])
{
	GpLogDriver_Web::Init();
	IGpLogDriver *logger = GpLogDriver_Web::GetInstance();

#if GP_ASYNCIFY_PARANOID
	SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
#endif

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
		return -1;

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Starting filesystem...");

	GpFileSystem_Web::GetInstance()->Init();

	GpDriverCollection *drivers = GpAppInterface_Get()->PL_GetDriverCollection();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Setting up drivers...");

	drivers->SetDriver<GpDriverIDs::kFileSystem>(GpFileSystem_Web::GetInstance());
	drivers->SetDriver<GpDriverIDs::kSystemServices>(GpSystemServices_Web::GetInstance());
	drivers->SetDriver<GpDriverIDs::kLog>(GpLogDriver_Web::GetInstance());

	g_gpGlobalConfig.m_displayDriverType = EGpDisplayDriverType_SDL_GL2;

	g_gpGlobalConfig.m_audioDriverType = EGpAudioDriverType_SDL2;

	g_gpGlobalConfig.m_fontHandlerType = EGpFontHandlerType_None;

	EGpInputDriverType inputDrivers[] =
	{
		EGpInputDriverType_SDL2_Gamepad
	};

	g_gpGlobalConfig.m_inputDriverTypes = inputDrivers;
	g_gpGlobalConfig.m_numInputDrivers = sizeof(inputDrivers) / sizeof(inputDrivers[0]);

	g_gpGlobalConfig.m_osGlobals = &g_gpXGlobals;
	g_gpGlobalConfig.m_logger = logger;
	g_gpGlobalConfig.m_systemServices = GpSystemServices_Web::GetInstance();

	GpDisplayDriverFactory::RegisterDisplayDriverFactory(EGpDisplayDriverType_SDL_GL2, GpDriver_CreateDisplayDriver_SDL_GL2);
	GpAudioDriverFactory::RegisterAudioDriverFactory(EGpAudioDriverType_SDL2, GpDriver_CreateAudioDriver_SDL);
	GpInputDriverFactory::RegisterInputDriverFactory(EGpInputDriverType_SDL2_Gamepad, GpDriver_CreateInputDriver_SDL2_Gamepad);

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "SDL environment configured, starting up");

	int returnCode = GpMain::Run();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "SDL environment exited with code %i, cleaning up", returnCode);

	SDL_Quit();

	return returnCode;
}
