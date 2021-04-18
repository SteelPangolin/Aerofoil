#include "PLInvisibleWidget.h"
#include "PLTimeTaggedVOSEvent.h"
#include "PLWidgets.h"

namespace PortabilityLayer
{
	InvisibleWidget::InvisibleWidget(const WidgetBasicState &state)
		: WidgetSpec<InvisibleWidget, WidgetTypes::kInvisible>(state)
		, m_clickable(state.m_enabled)
	{
	}

	InvisibleWidget::~InvisibleWidget()
	{
	}

	bool InvisibleWidget::Init(const WidgetBasicState &state, const void *additionalData)
	{
		(void)state;

		return true;
	}

	WidgetHandleState_t InvisibleWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		if (!m_visible || !m_enabled)
			return WidgetHandleStates::kIgnored;

		if (m_clickable && evt.IsLMouseDownEvent() && m_rect.Contains(m_window->MouseToLocal(evt.m_vosEvent.m_event.m_mouseInputEvent)))
			return WidgetHandleStates::kActivated;
		else
			return WidgetHandleStates::kIgnored;
	}

	int16_t InvisibleWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return DefaultCapture(captureContext, pos, callback);
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kInvisible, PortabilityLayer::InvisibleWidget)
