#pragma once
#ifndef __GPAPP_INTERFACE_H__
#define __GPAPP_INTERFACE_H__

#include "HostSuspendHook.h"
#include <stdint.h>

#include "GpDriverIndex.h"

#ifdef GP_APP_DLL

#ifdef GP_APP_DLL_EXPORT
#define GP_APP_DLL_EXPORT_API extern "C" __declspec(dllexport)
#else
#define GP_APP_DLL_EXPORT_API extern "C" __declspec(dllimport)
#endif

#else

#define GP_APP_DLL_EXPORT_API extern "C"

#endif

struct IGpAudioDriver;
struct IGpLogDriver;
struct IGpInputDriver;
struct IGpFontHandler;
struct GpDriverCollection;

struct IGpDisplayDriver;

class GpAppInterface
{
public:
	GP_ASYNCIFY_PARANOID_VIRTUAL void ApplicationInit() GP_ASYNCIFY_PARANOID_PURE;
	GP_ASYNCIFY_PARANOID_VIRTUAL int ApplicationMain() GP_ASYNCIFY_PARANOID_PURE;
	virtual void PL_IncrementTickCounter(uint32_t count) = 0;
	virtual void PL_Render(IGpDisplayDriver *displayDriver) = 0;
	virtual GpDriverCollection *PL_GetDriverCollection() = 0;

	virtual bool PL_AdjustRequestedResolution(uint32_t &physicalWidth, uint32_t &physicalHeight, uint32_t &virtualWidth, uint32_t &virtualheight, float &pixelScaleX, float &pixelScaleY) = 0;
};

GP_APP_DLL_EXPORT_API GpAppInterface *GpAppInterface_Get();

#endif
