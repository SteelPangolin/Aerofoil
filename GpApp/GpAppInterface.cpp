#include "GpAppInterface.h"

#include "DisplayDeviceManager.h"
#include "MenuManager.h"
#include "WindowManager.h"

#include "PLDrivers.h"
#include "PLSysCalls.h"

int gpAppMain();
void gpAppInit();


class GpAppInterfaceImpl final : public GpAppInterface
{
public:
	void ApplicationInit() GP_ASYNCIFY_PARANOID_OVERRIDE;
	int ApplicationMain() GP_ASYNCIFY_PARANOID_OVERRIDE;

	void PL_IncrementTickCounter(uint32_t count) override;
	void PL_Render(IGpDisplayDriver *displayDriver) override;
	GpDriverCollection *PL_GetDriverCollection() override;
	bool PL_AdjustRequestedResolution(uint32_t &physicalWidth, uint32_t &physicalHeight, uint32_t &virtualWidth, uint32_t &virtualheight, float &pixelScaleX, float &pixelScaleY) override;
};

void GpAppInterfaceImpl::ApplicationInit()
{
	gpAppInit();
}

int GpAppInterfaceImpl::ApplicationMain()
{
#if GP_ASYNCIFY_PARANOID
	return gpAppMain();
#else
	return PLSysCalls::MainExitWrapper(gpAppMain);
#endif
}

void GpAppInterfaceImpl::PL_IncrementTickCounter(uint32_t count)
{
	PortabilityLayer::DisplayDeviceManager::GetInstance()->IncrementTickCount(count);
}

void GpAppInterfaceImpl::PL_Render(IGpDisplayDriver *displayDriver)
{
	PortabilityLayer::WindowManager::GetInstance()->RenderFrame(displayDriver);
	PortabilityLayer::MenuManager::GetInstance()->RenderFrame(displayDriver);
}

GpDriverCollection *GpAppInterfaceImpl::PL_GetDriverCollection()
{
	return PLDrivers::GetDriverCollection();
}

bool GpAppInterfaceImpl::PL_AdjustRequestedResolution(uint32_t &physicalWidth, uint32_t &physicalHeight, uint32_t &virtualWidth, uint32_t &virtualheight, float &pixelScaleX, float &pixelScaleY)
{
	PortabilityLayer::DisplayDeviceManager::IResolutionChangeHandler *handler = PortabilityLayer::DisplayDeviceManager::GetInstance()->GetResolutionChangeHandler();

	if (!handler)
		return false;

	handler->AdjustRequestedResolution(physicalWidth, physicalHeight, virtualWidth, virtualheight, pixelScaleX, pixelScaleY);

	return true;
}


static GpAppInterfaceImpl gs_application;


GP_APP_DLL_EXPORT_API GpAppInterface *GpAppInterface_Get()
{
	return &gs_application;
}

#if GP_ASYNCIFY_PARANOID
void GpAppInterface::ApplicationInit()
{
	static_cast<GpAppInterfaceImpl*>(this)->ApplicationInit();
}

int GpAppInterface::ApplicationMain()
{
	return static_cast<GpAppInterfaceImpl*>(this)->ApplicationMain();
}
#endif
