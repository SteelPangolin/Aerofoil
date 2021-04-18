#pragma once

#include "PascalStr.h"
#include "PLWidgets.h"

struct Rect;

namespace PortabilityLayer
{
	class ScrollBarWidget final : public WidgetSpec<ScrollBarWidget, WidgetTypes::kScrollBar>
	{
	public:
		explicit ScrollBarWidget(const WidgetBasicState &state);

		bool Init(const WidgetBasicState &state, const void *additionalData) override;

		void OnEnabledChanged() override;
		WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt) GP_ASYNCIFY_PARANOID_OVERRIDE;
		void DrawControl(DrawSurface *surface) override;

		void SetState(int16_t state) override;
		void OnStateChanged() override;

		void SetMin(int32_t v) override;
		void SetMax(int32_t v) override;

		int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback) GP_ASYNCIFY_PARANOID_OVERRIDE;

		int ResolvePart(const Point &point) const override;

	private:
		bool IsHorizontal() const;
		bool Isvertical() const;

		void DrawControlHorizontal(DrawSurface *surface);
		void DrawControlVertical(DrawSurface *surface);

		void RefreshGrip();

		static void DrawBeveledBox(DrawSurface *surface, const Rect &rect);

		int16_t CaptureScrollSegment(void *captureContext, const Point &pos, int part, WidgetUpdateCallback_t callback);
		int16_t CaptureIndicator(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback);
		void IterateScrollSegment(void *captureContext, int part, WidgetUpdateCallback_t callback);

		int32_t m_min;
		int32_t m_max;
		int32_t m_gripSize;
		int32_t m_gripPos;
		int32_t m_laneCapacity;

		bool m_isActive;
		int m_activePart;

		WidgetUpdateCallback_t m_callback;
	};
}
