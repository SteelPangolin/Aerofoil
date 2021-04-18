#pragma once

#include "PascalStr.h"
#include "PLWidgets.h"

namespace PortabilityLayer
{
	class ButtonWidget final : public WidgetSpec<ButtonWidget, WidgetTypes::kButton>
	{
	public:
		enum ButtonStyle
		{
			kButtonStyle_Button,
			kButtonStyle_CheckBox,
			kButtonStyle_Radio,
		};

		struct AdditionalData
		{
			AdditionalData();

			ButtonStyle m_buttonStyle;
		};

		explicit ButtonWidget(const WidgetBasicState &state);

		bool Init(const WidgetBasicState &state, const void *additionalData) override;
		void DrawControl(DrawSurface *surface) override;

		void SetString(const PLPasStr &str) override;
		PLPasStr GetString() const override;

		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		void OnEnabledChanged() override;
		void OnStateChanged() override;
		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;
		void SetHighlightStyle(int16_t style, bool enabled) override;

		static void DrawDefaultButtonChrome(const Rect &rect, DrawSurface *surface);

	private:
		void DrawControlInternal(DrawSurface *surface, bool inverted);

		void DrawAsButton(DrawSurface *surface, bool inverted);
		void DrawAsRadio(DrawSurface *surface, bool inverted);
		void DrawAsCheck(DrawSurface *surface, bool inverted);

		PascalStr<255> m_text;
		ButtonStyle m_buttonStyle;
		bool m_haveHighlightOverride;
	};
}
