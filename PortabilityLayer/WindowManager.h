#pragma once

#include <stdint.h>

#include "PLRegions.h"
#include "PLSysCalls.h"

struct Window;
struct DrawSurface;
struct GDevice;
struct IGpDisplayDriver;
struct Point;
class PLPasStr;
struct Rect;
struct Window;

namespace PortabilityLayer
{
	struct WindowDef;
	struct Rect2i;
	struct Vec2i;

	class WindowManager
	{
	public:
		virtual void Init() = 0;
		virtual Window *GetPutInFrontSentinel() const = 0;
		virtual Window *CreateWindow(const WindowDef &windowDef) = 0;
		virtual void ResizeWindow(Window *window, int width, int height) = 0;
		virtual void MoveWindow(Window *window, int x, int y) = 0;
		virtual void PutWindowBehind(Window *window, Window *otherWindow) = 0;
		virtual void ShowWindow(Window *window) = 0;
		virtual void HideWindow(Window *window) = 0;
		virtual void FindWindow(const Point &point, Window **outWindow, short *outRegion) const = 0;
		virtual void DestroyWindow(Window *window) = 0;
		virtual void DragWindow(Window *window, const Point &startPoint, const Rect &constraintRect) = 0;
		virtual bool HandleCloseBoxClick(Window *window, const Point &startPoint) = 0;
		virtual void SetWindowTitle(Window *window, const PLPasStr &title) = 0;
		virtual Rect2i GetWindowFullRect(Window *window) const = 0;
		virtual bool GetWindowChromeInteractionZone(Window *window, const Vec2i &point, RegionID_t &outRegion) const = 0;
		virtual void SwapExclusiveWindow(Window *& windowRef) = 0;

		GP_ASYNCIFY_PARANOID_VIRTUAL void FlickerWindowIn(Window *window, int32_t velocity) GP_ASYNCIFY_PARANOID_PURE;
		GP_ASYNCIFY_PARANOID_VIRTUAL void FlickerWindowOut(Window *window, int32_t velocity) GP_ASYNCIFY_PARANOID_PURE;

		virtual void SetWindowDesaturation(Window *window, float desaturationLevel) = 0;

		virtual void SetResizeInProgress(Window *window, const PortabilityLayer::Vec2i &size) = 0;
		virtual void ClearResizeInProgress() = 0;

		virtual void RenderFrame(IGpDisplayDriver *displayDriver) = 0;

		virtual void HandleScreenResolutionChange(uint32_t prevWidth, uint32_t prevHeight, uint32_t newWidth, uint32_t newHeight) = 0;

		virtual void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b) = 0;

		virtual Vec2i GetDisplayResolution() const = 0;

		static WindowManager *GetInstance();
	};
}
