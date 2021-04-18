#include "PLImageWidget.h"
#include "PLQDraw.h"
#include "ResourceManager.h"

namespace PortabilityLayer
{
	ImageWidget::ImageWidget(const WidgetBasicState &state)
		: WidgetSpec<ImageWidget, WidgetTypes::kImage>(state)
	{
	}

	ImageWidget::~ImageWidget()
	{
	}

	bool ImageWidget::Init(const WidgetBasicState &state, const void *additionalData)
	{
		m_pict = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', state.m_resID).StaticCast<BitmapImage>();

		if (!m_pict)
			return false;

		return true;
	}

	void ImageWidget::DrawControl(DrawSurface *surface)
	{
		if (m_pict && m_rect.IsValid())
			surface->DrawPicture(m_pict, m_rect);
	}

	WidgetHandleState_t ImageWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		return DefaultProcessEvent(captureContext, evt);
	}

	int16_t ImageWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return DefaultCapture(captureContext, pos, callback);
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kImage, PortabilityLayer::ImageWidget)
