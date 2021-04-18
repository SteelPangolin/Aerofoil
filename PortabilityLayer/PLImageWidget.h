#pragma once

#include "PLWidgets.h"
#include "PLHandle.h"

struct BitmapImage;

namespace PortabilityLayer
{
	class ImageWidget final : public WidgetSpec<ImageWidget, WidgetTypes::kImage>
	{
	public:
		ImageWidget(const WidgetBasicState &state);
		~ImageWidget();

		bool Init(const WidgetBasicState &state, const void *additionalData) override;
		void DrawControl(DrawSurface *surface) override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

	private:
		THandle<BitmapImage> m_pict;
	};
}
