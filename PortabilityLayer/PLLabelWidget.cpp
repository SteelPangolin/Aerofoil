#include "PLLabelWidget.h"
#include "PLQDraw.h"
#include "FontFamily.h"
#include "PLStandardColors.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "ResolveCachingColor.h"

#include <algorithm>

namespace PortabilityLayer
{
	LabelWidget::LabelWidget(const WidgetBasicState &state)
		: WidgetSpec<LabelWidget, WidgetTypes::kLabel>(state)
		, m_text(state.m_text)
	{
	}

	bool LabelWidget::Init(const WidgetBasicState &state, const void *additionalData)
	{
		(void)state;
		return true;
	}

	void LabelWidget::SetString(const PLPasStr &str)
	{
		m_text.Set(str.Length(), str.Chars());
	}

	PLPasStr LabelWidget::GetString() const
	{
		return m_text.ToShortStr();
	}

	void LabelWidget::DrawControl(DrawSurface *surface)
	{
		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor blackColor  = StdColors::Black();

		surface->FillRect(m_rect, whiteColor);

		PortabilityLayer::RenderedFont *sysFont = GetFont(FontPresets::kSystem12Bold);

		const Point topLeftCorner = Point::Create(m_rect.left, m_rect.top);
		const Point textStartPoint = topLeftCorner + Point::Create(0, sysFont->GetMetrics().m_ascent);

		surface->DrawStringWrap(textStartPoint, m_rect, m_text.ToShortStr(), blackColor, sysFont);
	}

	WidgetHandleState_t LabelWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		return DefaultProcessEvent(captureContext, evt);
	}

	int16_t LabelWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return DefaultCapture(captureContext, pos, callback);
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kLabel, PortabilityLayer::LabelWidget)
