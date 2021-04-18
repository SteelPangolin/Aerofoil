#pragma once

#include "PLWidgets.h"

namespace PortabilityLayer
{
	class InvisibleWidget final : public WidgetSpec<InvisibleWidget, WidgetTypes::kInvisible>
	{
	public:
		explicit InvisibleWidget(const WidgetBasicState &state);
		~InvisibleWidget();

		bool Init(const WidgetBasicState &state, const void *additionalData) override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

	private:
		bool m_clickable;
	};
}
