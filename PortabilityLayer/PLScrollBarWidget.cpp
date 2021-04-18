#include "PLScrollBarWidget.h"
#include "PLControlDefinitions.h"
#include "PLStandardColors.h"
#include "PLTimeTaggedVOSEvent.h"
#include "ResolveCachingColor.h"

#include "PLRegions.h"

namespace PortabilityLayer
{
	ScrollBarWidget::ScrollBarWidget(const WidgetBasicState &state)
		: WidgetSpec<ScrollBarWidget, WidgetTypes::kScrollBar>(state)
		, m_min(0)
		, m_max(0)
		, m_gripSize(0)
		, m_gripPos(0)
		, m_laneCapacity(0)
		, m_isActive(false)
		, m_activePart(0)
		, m_callback(state.m_defaultCallback)
	{
	}

	WidgetHandleState_t ScrollBarWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		if (!m_visible || !m_enabled)
			return WidgetHandleStates::kIgnored;

		if (evt.IsLMouseDownEvent())
		{
			const Point pt = m_window->MouseToLocal(evt.m_vosEvent.m_event.m_mouseInputEvent);

			if (m_rect.Contains(pt))
			{
				if (Capture(captureContext, pt, m_callback) == RegionIDs::kNone)
					return WidgetHandleStates::kDigested;
				else
					return WidgetHandleStates::kActivated;
			}
			else
				return WidgetHandleStates::kIgnored;
		}

		return WidgetHandleStates::kIgnored;
	}

	void ScrollBarWidget::OnEnabledChanged()
	{
		DrawControl(m_window->GetDrawSurface());
	}

	bool ScrollBarWidget::Init(const WidgetBasicState &state, const void *additionalData)
	{
		m_min = state.m_min;
		m_max = state.m_max;

		RefreshGrip();

		return true;
	}

	void ScrollBarWidget::DrawControl(DrawSurface *surface)
	{
		ResolveCachingColor whiteColor = StdColors::White();
		surface->FillRect(this->m_rect.Inset(1, 1), whiteColor);

		if (m_rect.Width() < 16 || m_rect.Height() < 16)
			return;

		if (IsHorizontal())
			DrawControlHorizontal(surface);
		else
			DrawControlVertical(surface);
	}

	void ScrollBarWidget::DrawControlHorizontal(DrawSurface *surface)
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor midGrayColor = RGBAColor::Create(136, 136, 136, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(187, 187, 187, 255);
		ResolveCachingColor darkGrayColor = RGBAColor::Create(85, 85, 85, 255);

		ResolveCachingColor *leftArrowColor = &blackColor;
		ResolveCachingColor *rightArrowColor = &blackColor;

		surface->FrameRect(m_rect, blackColor);

		const Rect leftArrowRect = Rect::Create(m_rect.top, m_rect.left, m_rect.bottom, m_rect.left + 16);
		DrawBeveledBox(surface, leftArrowRect);

		if (m_isActive && m_activePart == kControlUpButtonPart)
		{
			surface->FillRect(leftArrowRect.Inset(1, 1), darkGrayColor);
			leftArrowColor = &whiteColor;
		}

		for (int i = 0; i < 4; i++)
		{
			const Rect arrowSegRect = Rect::Create(7 - i + leftArrowRect.top, 6 + i + leftArrowRect.left, 9 + i + leftArrowRect.top, 7 + i + leftArrowRect.left);
			surface->FillRect(arrowSegRect, *leftArrowColor);
		}

		const Rect rightArrowRect = Rect::Create(m_rect.top, m_rect.right - 16, m_rect.bottom, m_rect.right);
		DrawBeveledBox(surface, rightArrowRect);

		if (m_isActive && m_activePart == kControlDownButtonPart)
		{
			surface->FillRect(rightArrowRect.Inset(1, 1), darkGrayColor);
			rightArrowColor = &whiteColor;
		}

		for (int i = 0; i < 4; i++)
		{
			const Rect arrowSegRect = Rect::Create(4 + i + rightArrowRect.top, 6 + i + rightArrowRect.left, 12 - i + rightArrowRect.top, 7 + i + rightArrowRect.left);
			surface->FillRect(arrowSegRect, blackColor);
		}

		const Rect laneRect = Rect::Create(m_rect.top, leftArrowRect.right, m_rect.bottom, rightArrowRect.left);

		surface->FillRect(Rect::Create(laneRect.top + 1, laneRect.left, laneRect.top + 2, laneRect.right), midGrayColor);

		surface->FillRect(Rect::Create(laneRect.top + 2, laneRect.left, laneRect.bottom - 2, laneRect.right), lightGrayColor);

		surface->FillRect(Rect::Create(laneRect.bottom - 2, laneRect.left, laneRect.bottom - 1, laneRect.right), whiteColor);

		if (m_isActive && m_activePart == kControlPageUpPart)
			surface->FillRect(Rect::Create(laneRect.top, laneRect.left, laneRect.bottom, laneRect.left + m_gripPos), darkGrayColor);

		if (m_isActive && m_activePart == kControlPageDownPart)
			surface->FillRect(Rect::Create(laneRect.top, laneRect.left + m_gripPos, laneRect.bottom, laneRect.right), darkGrayColor);

		surface->FillRect(Rect::Create(laneRect.top, laneRect.left, laneRect.top + 1, laneRect.right), blackColor);
		surface->FillRect(Rect::Create(laneRect.bottom - 1, laneRect.left, laneRect.bottom, laneRect.right), blackColor);


		if (m_laneCapacity > 0)
			DrawBeveledBox(surface, Rect::Create(laneRect.top, laneRect.left + m_gripPos, laneRect.bottom, laneRect.left + m_gripPos + m_gripSize));
	}

	void ScrollBarWidget::DrawControlVertical(DrawSurface *surface)
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor midGrayColor = RGBAColor::Create(136, 136, 136, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(187, 187, 187, 255);
		ResolveCachingColor darkGrayColor = RGBAColor::Create(85, 85, 85, 255);

		surface->FrameRect(m_rect, blackColor);


		ResolveCachingColor *topArrowColor = &blackColor;
		ResolveCachingColor *bottomArrowColor = &blackColor;

		const Rect topArrowRect = Rect::Create(m_rect.top, m_rect.left, m_rect.top + 16, m_rect.right);
		DrawBeveledBox(surface, topArrowRect);

		if (m_isActive && m_activePart == kControlUpButtonPart)
		{
			surface->FillRect(topArrowRect.Inset(1, 1), darkGrayColor);
			topArrowColor = &whiteColor;
		}

		for (int i = 0; i < 4; i++)
		{
			const Rect arrowSegRect = Rect::Create(6 + i + topArrowRect.top, 7 - i + topArrowRect.left, 7 + i + topArrowRect.top, 9 + i + topArrowRect.left);
			surface->FillRect(arrowSegRect, *topArrowColor);
		}

		const Rect bottomArrowRect = Rect::Create(m_rect.bottom - 16, m_rect.left, m_rect.bottom, m_rect.right);
		DrawBeveledBox(surface, bottomArrowRect);

		if (m_isActive && m_activePart == kControlDownButtonPart)
		{
			surface->FillRect(bottomArrowRect.Inset(1, 1), darkGrayColor);
			bottomArrowColor = &whiteColor;
		}

		for (int i = 0; i < 4; i++)
		{
			const Rect arrowSegRect = Rect::Create(6 + i + bottomArrowRect.top, 4 + i + bottomArrowRect.left, 7 + i + bottomArrowRect.top, 12 - i + bottomArrowRect.left);
			surface->FillRect(arrowSegRect, *bottomArrowColor);
		}

		const Rect laneRect = Rect::Create(topArrowRect.bottom, m_rect.left, bottomArrowRect.top, m_rect.right);

		surface->FillRect(Rect::Create(laneRect.top, laneRect.left + 1, laneRect.bottom, laneRect.left + 2), midGrayColor);
		surface->FillRect(Rect::Create(laneRect.top, laneRect.left + 2, laneRect.bottom, laneRect.right - 2), lightGrayColor);
		surface->FillRect(Rect::Create(laneRect.bottom, laneRect.right - 2, laneRect.bottom, laneRect.right - 1), whiteColor);

		if (m_isActive && m_activePart == kControlPageUpPart)
			surface->FillRect(Rect::Create(laneRect.top, laneRect.left, laneRect.top + m_gripPos, laneRect.right), darkGrayColor);

		if (m_isActive && m_activePart == kControlPageDownPart)
			surface->FillRect(Rect::Create(laneRect.top + m_gripPos, laneRect.left, laneRect.bottom, laneRect.right), darkGrayColor);

		surface->FillRect(Rect::Create(laneRect.top, laneRect.left, laneRect.bottom, laneRect.left + 1), blackColor);
		surface->FillRect(Rect::Create(laneRect.top, laneRect.right - 1, laneRect.bottom, laneRect.right), blackColor);

		if (m_laneCapacity > 0)
			DrawBeveledBox(surface, Rect::Create(laneRect.top + m_gripPos, laneRect.left, laneRect.top + m_gripPos + m_gripSize, laneRect.right));
	}

	void ScrollBarWidget::DrawBeveledBox(DrawSurface *surface, const Rect &rect)
	{
		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor midGrayColor = RGBAColor::Create(136, 136, 136, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(187, 187, 187, 255);

		surface->FrameRect(rect, blackColor);

		surface->FillRect(rect.Inset(1, 1), lightGrayColor);

		surface->FillRect(Rect::Create(rect.top + 1, rect.left + 1, rect.top + 2, rect.right - 2), whiteColor);
		surface->FillRect(Rect::Create(rect.top + 2, rect.left + 1, rect.bottom - 2, rect.left + 2), whiteColor);
		surface->FillRect(Rect::Create(rect.bottom - 2, rect.left + 2, rect.bottom - 1, rect.right - 1), midGrayColor);
		surface->FillRect(Rect::Create(rect.top + 2, rect.right - 2, rect.bottom - 2, rect.right - 1), midGrayColor);
	}

	bool ScrollBarWidget::IsHorizontal() const
	{
		return m_rect.Width() > m_rect.Height();
	}

	bool ScrollBarWidget::Isvertical() const
	{
		return !this->IsHorizontal();
	}

	void ScrollBarWidget::RefreshGrip()
	{
		m_gripSize = 16;
		m_laneCapacity = 0;
		m_gripPos = 0;

		if (m_gripSize > 0 && m_min < m_max)
		{
			if (IsHorizontal())
				m_laneCapacity = static_cast<int32_t>(m_rect.Width() - 32) - m_gripSize;
			else
				m_laneCapacity = static_cast<int32_t>(m_rect.Height() - 32) - m_gripSize;

			if (m_laneCapacity < 0)
				m_laneCapacity = 0;

			if (m_laneCapacity > 0)
			{
				m_gripPos = 0;
				if (m_state >= m_max)
					m_gripPos = m_laneCapacity;
				else if (m_state > m_min)
					m_gripPos = (m_state * m_laneCapacity) / (m_max - m_min);
			}
			else
				m_gripPos = 0;
		}
	}

	void ScrollBarWidget::SetState(int16_t state)
	{
		if (state < m_min)
			WidgetSpec<ScrollBarWidget, WidgetTypes::kScrollBar>::SetState(m_min);
		else if (state > m_max)
			WidgetSpec<ScrollBarWidget, WidgetTypes::kScrollBar>::SetState(m_max);
		else
			WidgetSpec<ScrollBarWidget, WidgetTypes::kScrollBar>::SetState(state);
	}

	void ScrollBarWidget::OnStateChanged()
	{
		RefreshGrip();
		DrawControl(m_window->GetDrawSurface());
	}

	void ScrollBarWidget::SetMin(int32_t v)
	{
		m_min = v;
		if (m_state < m_min)
			SetState(m_min);
		else
		{
			RefreshGrip();
			DrawControl(m_window->GetDrawSurface());
		}
	}

	void ScrollBarWidget::SetMax(int32_t v)
	{
		m_max = v;
		if (m_state > m_max)
			SetState(m_max);
		else
		{
			RefreshGrip();
			DrawControl(m_window->GetDrawSurface());
		}
	}

	int16_t ScrollBarWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		int part = ResolvePart(pos);
		if (!part)
			return 0;

		if (part == kControlIndicatorPart)
			return CaptureIndicator(captureContext, pos, callback);
		else
			return CaptureScrollSegment(captureContext, pos, part, callback);
	}

	int16_t ScrollBarWidget::CaptureScrollSegment(void *captureContext, const Point &pos, int part, WidgetUpdateCallback_t callback)
	{
		int tickDelay = 15;

		Point currentPos = pos;
		bool wasActive = false;

		m_activePart = part;

		int ticksUntilIterate = 0;
		for (;;)
		{
			m_isActive = (ResolvePart(currentPos) == part);	// Update in-bounds since the scroll may invalidate it

			if (ticksUntilIterate == 0)
			{
				if (m_isActive)
					IterateScrollSegment(captureContext, part, callback);

				ticksUntilIterate = tickDelay;
			}

			if (m_isActive != wasActive)
			{
				wasActive = m_isActive;
				ticksUntilIterate = tickDelay;

				DrawControl(m_window->GetDrawSurface());
			}

			TimeTaggedVOSEvent evt;
			if (WaitForEvent(&evt, 1))
			{
				if (evt.m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
				{
					const GpMouseInputEvent &mouseEvt = evt.m_vosEvent.m_event.m_mouseInputEvent;
					if (mouseEvt.m_button == GpMouseButtons::kLeft && mouseEvt.m_eventType == GpMouseEventTypes::kUp)
					{
						if (m_isActive)
						{
							m_isActive = false;
							DrawControl(m_window->GetDrawSurface());
						}

						return part;
					}
					else
					{
						currentPos = m_window->MouseToLocal(mouseEvt);
						m_isActive = (ResolvePart(currentPos) == part);
					}
				}
			}
			else
				ticksUntilIterate--;
		}
	}

	int16_t ScrollBarWidget::CaptureIndicator(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		const bool isHorizontal = IsHorizontal();

		const int32_t startCoordinate = isHorizontal ? pos.h : pos.v;
		int32_t currentGripPos = m_gripPos;

		Rect currentHandle;
		if (isHorizontal)
			currentHandle = Rect::Create(m_rect.top, m_rect.left + 16 + m_gripPos, m_rect.bottom, m_rect.left + 16 + m_gripPos + m_gripSize);
		else
			currentHandle = Rect::Create(m_rect.top + 16 + m_gripPos, m_rect.left, m_rect.top + 16 + m_gripPos + m_gripSize, m_rect.right);

		DrawSurface *surface = this->m_window->GetDrawSurface();

		uint8_t solidPattern[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
		surface->InvertFillRect(currentHandle, solidPattern);

		for (;;)
		{
			TimeTaggedVOSEvent evt;
			if (WaitForEvent(&evt, 1))
			{
				if (evt.m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
				{
					const GpMouseInputEvent &mouseEvt = evt.m_vosEvent.m_event.m_mouseInputEvent;
					if (mouseEvt.m_eventType == GpMouseEventTypes::kUp || mouseEvt.m_eventType == GpMouseEventTypes::kMove)
					{
						const Point localPoint = m_window->MouseToLocal(mouseEvt);
						int32_t desiredCoordinate = isHorizontal ? localPoint.h : localPoint.v;
						int32_t desiredGripPos = m_gripPos + (desiredCoordinate - startCoordinate);

						if (desiredGripPos < 0)
							desiredGripPos = 0;
						else if (desiredGripPos > m_laneCapacity)
							desiredGripPos = m_laneCapacity;

						if (desiredGripPos != currentGripPos)
						{
							surface->InvertFillRect(currentHandle, solidPattern);

							if (isHorizontal)
								currentHandle = Rect::Create(m_rect.top, m_rect.left + 16 + desiredGripPos, m_rect.bottom, m_rect.left + 16 + desiredGripPos + m_gripSize);
							else
								currentHandle = Rect::Create(m_rect.top + 16 + desiredGripPos, m_rect.left, m_rect.top + 16 + desiredGripPos + m_gripSize, m_rect.right);

							surface->InvertFillRect(currentHandle, solidPattern);

							currentGripPos = desiredGripPos;
						}
					}

					if (mouseEvt.m_eventType == GpMouseEventTypes::kUp && mouseEvt.m_button == GpMouseButtons::kLeft)
					{
						surface->InvertFillRect(currentHandle, solidPattern);

						//int32_t desiredState = (currentGripPos / m_laneCapacity) * (m_max - m_min);
						int32_t desiredState = m_min;

						if (m_laneCapacity > 0)
						{
							desiredState = ((currentGripPos * (m_max - m_min)) * 2 + m_laneCapacity) / (m_laneCapacity * 2);

							// This shouldn't happen unless something weird happens like an int overflow
							if (desiredState < m_min)
								desiredState = m_min;
							else if (desiredState > m_max)
								desiredState = m_max;

							SetState(desiredState);
						}

						return kControlIndicatorPart;
					}
				}
			}
		}
	}

	void ScrollBarWidget::IterateScrollSegment(void *captureContext, int part, WidgetUpdateCallback_t callback)
	{
		if (callback != nullptr)
			callback(captureContext, this, part);
	}

	int ScrollBarWidget::ResolvePart(const Point &point) const
	{
		if (!m_rect.Contains(point))
			return 0;

		int32_t span = 0;
		int32_t coord = 0;
		const bool isHorizontal = IsHorizontal();
		if (isHorizontal)
		{
			span = m_rect.Width();
			coord = point.h - m_rect.left;
		}
		else
		{
			span = m_rect.Height();
			coord = point.v - m_rect.top;
		}

		if (coord < 16)
			return kControlUpButtonPart;

		if (coord - 16 < m_gripPos)
			return kControlPageUpPart;

		if (coord - 16 - m_gripPos < m_gripSize)
			return kControlIndicatorPart;

		if (coord < span - 16)
			return kControlPageDownPart;

		return kControlDownButtonPart;
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kScrollBar, PortabilityLayer::ScrollBarWidget)
