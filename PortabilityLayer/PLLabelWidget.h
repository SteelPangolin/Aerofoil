#pragma once

#include "PascalStr.h"
#include "PLWidgets.h"

namespace PortabilityLayer
{
	class LabelWidget final : public WidgetSpec<LabelWidget, WidgetTypes::kLabel>
	{
	public:
		LabelWidget(const WidgetBasicState &state);

		bool Init(const WidgetBasicState &state, const void *additionalData) override;

		void SetString(const PLPasStr &str) override;
		PLPasStr GetString() const override;

		void DrawControl(DrawSurface *surface) override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

	private:
		PascalStr<255> m_text;
	};
}
