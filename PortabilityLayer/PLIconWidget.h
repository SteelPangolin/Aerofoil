#pragma once

#include "PLWidgets.h"
#include "PLHandle.h"

namespace PortabilityLayer
{
	class PixMapImpl;

	class IconWidget final : public WidgetSpec<IconWidget, WidgetTypes::kIcon>
	{
	public:
		IconWidget(const WidgetBasicState &state);
		~IconWidget() override;

		bool Init(const WidgetBasicState &state, const void *additionalData) override;

		void DrawControl(DrawSurface *surface) override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

	private:
		THandle<PixMapImpl> m_iconImage;
		THandle<PixMapImpl> m_iconMask;
	};
}
