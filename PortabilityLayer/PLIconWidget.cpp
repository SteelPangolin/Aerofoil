#include "PLIconWidget.h"

#include "IconLoader.h"
#include "QDPixMap.h"
#include "PLTimeTaggedVOSEvent.h"

#include <assert.h>

namespace PortabilityLayer
{
	IconWidget::IconWidget(const WidgetBasicState &state)
		: WidgetSpec<IconWidget, WidgetTypes::kIcon>(state)
	{
	}

	IconWidget::~IconWidget()
	{
		if (m_iconImage)
			PixMapImpl::Destroy(m_iconImage);
		if (m_iconMask)
			PixMapImpl::Destroy(m_iconMask);
	}

	bool IconWidget::Init(const WidgetBasicState &state, const void *additionalData)
	{
		PL_DEAD(str);

		THandle<PixMapImpl> colorImage;
		THandle<PixMapImpl> bwImage;
		THandle<PixMapImpl> maskImage;

		if (!PortabilityLayer::IconLoader::GetInstance()->LoadColorIcon(state.m_resID, colorImage, bwImage, maskImage))
			return false;

		PixMapImpl::Destroy(bwImage);
		m_iconImage = colorImage;
		m_iconMask = maskImage;

		assert(m_iconImage != nullptr);
		assert(m_iconMask != nullptr);

		return true;
	}

	void IconWidget::DrawControl(DrawSurface *surface)
	{
		CopyMask(*m_iconImage, *m_iconMask, *surface->m_port.GetPixMap(), &(*m_iconImage)->m_rect, &(*m_iconMask)->m_rect, &m_rect);
		surface->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
	}

	WidgetHandleState_t IconWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		if (!m_visible || !m_enabled)
			return WidgetHandleStates::kIgnored;

		if (evt.IsLMouseDownEvent() && m_rect.Contains(m_window->MouseToLocal(evt.m_vosEvent.m_event.m_mouseInputEvent)))
			return WidgetHandleStates::kActivated;
		else
			return WidgetHandleStates::kIgnored;
	}

	int16_t IconWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return DefaultCapture(captureContext, pos, callback);
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kIcon, PortabilityLayer::IconWidget)
