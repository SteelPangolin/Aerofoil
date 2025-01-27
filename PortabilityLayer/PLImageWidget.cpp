#include "PLImageWidget.h"
#include "PLQDraw.h"
#include "ResourceManager.h"

namespace PortabilityLayer
{
	ImageWidget::ImageWidget(const WidgetBasicState &state)
		: WidgetSpec<ImageWidget>(state)
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
}
