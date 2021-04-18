#include "PLButtonWidget.h"
#include "PLCore.h"
#include "PLControlDefinitions.h"
#include "PLQDraw.h"
#include "PLRegions.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"
#include "PLTimeTaggedVOSEvent.h"
#include "FontFamily.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "ResolveCachingColor.h"
#include "SimpleGraphic.h"

#include <algorithm>

static const int kButtonLightGray = 238;
static const int kButtonMidGray = 221;
static const int kButtonMidDarkGray = 170;
static const int kButtonDarkGray = 102;

#ifdef CLR
#undef CLR
#endif

#define CLR(n) { n, n, n, 255}

static const PortabilityLayer::RGBAColor gs_buttonTopLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(kButtonMidGray),
	CLR(0), CLR(kButtonMidGray), CLR(255)
};

static const PortabilityLayer::RGBAColor gs_buttonTopRightCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0),
	CLR(kButtonMidGray), CLR(0), CLR(0),
	CLR(255), CLR(kButtonMidGray), CLR(0)
};

static const PortabilityLayer::RGBAColor gs_buttonBottomLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(kButtonMidGray), CLR(255),
	CLR(0), CLR(0), CLR(kButtonMidGray),
	CLR(0), CLR(0), CLR(0)
};

static const PortabilityLayer::RGBAColor gs_buttonBottomRightCornerGraphicPixels[] =
{
	CLR(kButtonMidGray), CLR(kButtonDarkGray), CLR(0),
	CLR(kButtonDarkGray), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(0)
};

// Pressed

static const PortabilityLayer::RGBAColor gs_buttonPressedTopLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(kButtonDarkGray),
	CLR(0), CLR(kButtonDarkGray), CLR(kButtonDarkGray)
};

static const PortabilityLayer::RGBAColor gs_buttonPressedTopRightCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0),
	CLR(kButtonDarkGray), CLR(0), CLR(0),
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(0)
};

static const PortabilityLayer::RGBAColor gs_buttonPressedBottomLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(kButtonDarkGray), CLR(kButtonDarkGray),
	CLR(0), CLR(0), CLR(kButtonDarkGray),
	CLR(0), CLR(0), CLR(0)
};

static const PortabilityLayer::RGBAColor gs_buttonPressedBottomRightCornerGraphicPixels[] =
{
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(0),
	CLR(kButtonDarkGray), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(0)
};

// Disabled

static const PortabilityLayer::RGBAColor gs_buttonDisabledTopLeftCornerGraphicPixels[] =
{
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray),
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonLightGray),
	CLR(kButtonDarkGray), CLR(kButtonLightGray), CLR(kButtonLightGray)
};

static const PortabilityLayer::RGBAColor gs_buttonDisabledTopRightCornerGraphicPixels[] =
{
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray),
	CLR(kButtonLightGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray),
	CLR(kButtonLightGray), CLR(kButtonLightGray), CLR(kButtonDarkGray)
};

static const PortabilityLayer::RGBAColor gs_buttonDisabledBottomLeftCornerGraphicPixels[] =
{
	CLR(kButtonDarkGray), CLR(kButtonLightGray), CLR(kButtonLightGray),
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonLightGray),
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray)
};

static const PortabilityLayer::RGBAColor gs_buttonDisabledBottomRightCornerGraphicPixels[] =
{
	CLR(kButtonLightGray), CLR(kButtonLightGray), CLR(kButtonDarkGray),
	CLR(kButtonLightGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray),
	CLR(kButtonDarkGray), CLR(kButtonDarkGray), CLR(kButtonDarkGray)
};

// Default boundary

static const PortabilityLayer::RGBAColor gs_buttonDefaultTopLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(255),
	CLR(0), CLR(0), CLR(0), CLR(255), CLR(kButtonMidGray),
	CLR(0), CLR(0), CLR(255), CLR(kButtonMidGray), CLR(kButtonMidGray),
	CLR(0), CLR(255), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(0),
};

static const PortabilityLayer::RGBAColor gs_buttonDefaultTopRightCornerGraphicPixels[] =
{
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(0),
	CLR(255), CLR(0), CLR(0), CLR(0), CLR(0),
	CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(0), CLR(0), CLR(0),
	CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(0), CLR(0),
	CLR(0), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(kButtonDarkGray), CLR(0),
};

static const PortabilityLayer::RGBAColor gs_buttonDefaultBottomLeftCornerGraphicPixels[] =
{
	CLR(0), CLR(255), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(0),
	CLR(0), CLR(0), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(kButtonMidGray),
	CLR(0), CLR(0), CLR(0), CLR(kButtonMidGray), CLR(kButtonMidGray),
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(kButtonDarkGray),
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(0),
};

static const PortabilityLayer::RGBAColor gs_buttonDefaultBottomRightCornerGraphicPixels[] =
{
	CLR(0), CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(kButtonDarkGray), CLR(0),
	CLR(kButtonMidGray), CLR(kButtonMidGray), CLR(kButtonDarkGray), CLR(0), CLR(0),
	CLR(kButtonMidGray), CLR(kButtonDarkGray), CLR(0), CLR(0), CLR(0),
	CLR(kButtonDarkGray), CLR(0), CLR(0), CLR(0), CLR(0),
	CLR(0), CLR(0), CLR(0), CLR(0), CLR(0),
};

static const PortabilityLayer::RGBAColor gs_buttonRadioGraphicPixels[] =
{
	CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),
	CLR(0),CLR(0),CLR(0),CLR(255),CLR(255),CLR(255),CLR(255),CLR(255),CLR(255),CLR(0),CLR(0),CLR(0),
	CLR(0),CLR(0),CLR(255),CLR(255),CLR(kButtonLightGray),CLR(kButtonLightGray),CLR(kButtonLightGray),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(0),CLR(0),
	CLR(0),CLR(255),CLR(255),CLR(kButtonLightGray),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(255),CLR(kButtonLightGray),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(255),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(255),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(255),CLR(kButtonLightGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(255),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(0),
	CLR(0),CLR(0),CLR(kButtonMidGray),CLR(kButtonMidGray),CLR(kButtonMidDarkGray),CLR(kButtonMidDarkGray),CLR(kButtonMidDarkGray),CLR(kButtonMidDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(0),CLR(0),
	CLR(0),CLR(0),CLR(0),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(kButtonDarkGray),CLR(0),CLR(0),CLR(0),
	CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),CLR(0),
};


static PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3> gs_buttonCornerGraphics[4] =
{
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonTopLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonTopRightCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonBottomLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonBottomRightCornerGraphicPixels),
};

static PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3> gs_buttonPressedCornerGraphics[4] =
{
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonPressedTopLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonPressedTopRightCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonPressedBottomLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonPressedBottomRightCornerGraphicPixels),
};

static PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3> gs_buttonDisabledCornerGraphics[4] =
{
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonDisabledTopLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonDisabledTopRightCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonDisabledBottomLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3>(gs_buttonDisabledBottomRightCornerGraphicPixels),
};

static PortabilityLayer::SimpleGraphicInstanceRGBA<5, 5> gs_buttonDefaultCornerGraphics[4] =
{
	PortabilityLayer::SimpleGraphicInstanceRGBA<5, 5>(gs_buttonDefaultTopLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<5, 5>(gs_buttonDefaultTopRightCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<5, 5>(gs_buttonDefaultBottomLeftCornerGraphicPixels),
	PortabilityLayer::SimpleGraphicInstanceRGBA<5, 5>(gs_buttonDefaultBottomRightCornerGraphicPixels),
};

static const uint8_t gs_buttonTopLeftGraphicMask[] = { 0x2f, 0xff };
static const uint8_t gs_buttonTopRightGraphicMask[] = { 0x9b, 0xff };
static const uint8_t gs_buttonBottomLeftGraphicMask[] = { 0xec, 0xff };
static const uint8_t gs_buttonBottomRightGraphicMask[] = { 0xfa, 0x7f };

static const uint8_t gs_buttonDefaultTopLeftGraphicMask[] = { 0x08, 0xCE, 0xFF, 0xFF };
static const uint8_t gs_buttonDefaultTopRightGraphicMask[] = { 0x86, 0x39, 0xEF, 0xFF };
static const uint8_t gs_buttonDefaultBottomLeftGraphicMask[] = { 0xFB, 0xCE, 0x30, 0xFF };
static const uint8_t gs_buttonDefaultBottomRightGraphicMask[] = { 0xFF, 0xB9, 0x88, 0x7F };

static const uint8_t gs_buttonRadioGraphicMask[] = { 0x1f, 0x83, 0xFC, 0x7F, 0xEF, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xF7, 0xFE, 0x3F, 0xC1, 0xF8 };
static PortabilityLayer::SimpleGraphicInstanceRGBA<12, 12> gs_buttonRadioGraphic(gs_buttonRadioGraphicPixels);


namespace PortabilityLayer
{

	ButtonWidget::AdditionalData::AdditionalData()
		: m_buttonStyle(kButtonStyle_Button)
	{
	}

	ButtonWidget::ButtonWidget(const WidgetBasicState &state)
		: WidgetSpec<ButtonWidget, WidgetTypes::kButton>(state)
		, m_text(state.m_text)
		, m_haveHighlightOverride(false)
		, m_buttonStyle(kButtonStyle_Button)
	{
	}

	WidgetHandleState_t ButtonWidget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		if (!m_visible || !m_enabled)
			return WidgetHandleStates::kIgnored;

		if (evt.IsLMouseDownEvent())
		{
			const Point pt = m_window->MouseToLocal(evt.m_vosEvent.m_event.m_mouseInputEvent);

			if (m_rect.Contains(pt))
			{
				if (Capture(captureContext, pt, nullptr) == RegionIDs::kNone)
					return WidgetHandleStates::kDigested;
				else
					return WidgetHandleStates::kActivated;
			}
			else
				return WidgetHandleStates::kIgnored;
		}

		return WidgetHandleStates::kIgnored;
	}

	void ButtonWidget::OnEnabledChanged()
	{
		if (m_window)
			DrawControl(m_window->GetDrawSurface());
	}

	void ButtonWidget::OnStateChanged()
	{
		if (m_window)
			DrawControl(m_window->GetDrawSurface());
	}

	int16_t ButtonWidget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		if (!m_enabled || !m_visible)
			return 0;

		bool drawingDownState = false;
		bool isInBounds = m_rect.Contains(pos);
		for (;;)
		{
			if (drawingDownState != isInBounds)
			{
				DrawControlInternal(m_window->GetDrawSurface(), isInBounds);
				drawingDownState = isInBounds;
			}

			TimeTaggedVOSEvent evt;
			bool haveEvent = false;
			{
				PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
				haveEvent = WaitForEvent(&evt, 1);
			}

			if (haveEvent)
			{
				if (evt.m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
				{
					const Point pt = m_window->MouseToLocal(evt.m_vosEvent.m_event.m_mouseInputEvent);
					isInBounds = m_rect.Contains(pt);

					if (evt.IsLMouseUpEvent())
					{
						DrawControlInternal(m_window->GetDrawSurface(), false);

						if (isInBounds)
							return RegionIDs::kContent;
						else
							return RegionIDs::kNone;
					}
				}
			}
		}
	}

	bool ButtonWidget::Init(const WidgetBasicState &state, const void *additionalDataPtr)
	{
		const AdditionalData &additionalData = *static_cast<const AdditionalData *>(additionalDataPtr);

		m_buttonStyle = additionalData.m_buttonStyle;

		return true;
	}

	void ButtonWidget::DrawControl(DrawSurface *surface)
	{
		DrawControlInternal(surface, false);
	}


	void ButtonWidget::SetString(const PLPasStr &str)
	{
		m_text = PascalStr<255>(str);
	}

	PLPasStr ButtonWidget::GetString() const
	{
		return m_text.ToShortStr();
	}

	void ButtonWidget::DrawControlInternal(DrawSurface *surface, bool inverted)
	{
		switch (m_buttonStyle)
		{
		case kButtonStyle_Button:
			DrawAsButton(surface, inverted);
			break;
		case kButtonStyle_CheckBox:
			DrawAsCheck(surface, inverted);
			break;
		case kButtonStyle_Radio:
			DrawAsRadio(surface, inverted);
			break;
		default:
			break;
		}
	}

	void ButtonWidget::DrawAsButton(DrawSurface *surface, bool inverted)
	{
		const Rect rect = m_rect;

		PixMap **pixMap = surface->m_port.GetPixMap();

		const PortabilityLayer::SimpleGraphicInstanceRGBA<3, 3> *cornerGraphics = nullptr;

		PortabilityLayer::RGBAColor leftStripeColors[2];
		PortabilityLayer::RGBAColor rightStripeColors[2];
		PortabilityLayer::RGBAColor topStripeColors[2];
		PortabilityLayer::RGBAColor bottomStripeColors[2];
		PortabilityLayer::RGBAColor centerColor;
		PortabilityLayer::RGBAColor textColor;
		PortabilityLayer::RGBAColor borderColor;

		if (!m_enabled)
		{
			cornerGraphics = gs_buttonDisabledCornerGraphics;
			topStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			topStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			leftStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			leftStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			bottomStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			bottomStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			rightStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			rightStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			centerColor = PortabilityLayer::RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
			textColor = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			borderColor = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
		}
		else if (inverted || m_haveHighlightOverride)
		{
			cornerGraphics = gs_buttonPressedCornerGraphics;
			topStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			topStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			leftStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			leftStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			bottomStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			bottomStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			rightStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			rightStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			centerColor = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			textColor = StdColors::White();
			borderColor = StdColors::Black();
		}
		else
		{
			cornerGraphics = gs_buttonCornerGraphics;
			topStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
			topStripeColors[1] = StdColors::White();
			leftStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
			leftStripeColors[1] = StdColors::White();
			bottomStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			bottomStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
			rightStripeColors[0] = PortabilityLayer::RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
			rightStripeColors[1] = PortabilityLayer::RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
			centerColor = PortabilityLayer::RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
			textColor = StdColors::Black();
			borderColor = StdColors::Black();
		}


		cornerGraphics[0].DrawToPixMapWithMask(pixMap, gs_buttonTopLeftGraphicMask, rect.left, rect.top);
		cornerGraphics[1].DrawToPixMapWithMask(pixMap, gs_buttonTopRightGraphicMask, rect.right - 3, rect.top);
		cornerGraphics[2].DrawToPixMapWithMask(pixMap, gs_buttonBottomLeftGraphicMask, rect.left, rect.bottom - 3);
		cornerGraphics[3].DrawToPixMapWithMask(pixMap, gs_buttonBottomRightGraphicMask, rect.right - 3, rect.bottom - 3);

		for (int i = 0; i < 3; i++)
		{
			ResolveCachingColor color;

			if (i == 0)
				color = borderColor;

			if (i != 0)
				color = leftStripeColors[i - 1];

			surface->FillRect(Rect::Create(rect.top + 3, rect.left + i, rect.bottom - 3, rect.left + i + 1), color);

			if (i != 0)
				color = rightStripeColors[i - 1];

			surface->FillRect(Rect::Create(rect.top + 3, rect.right - 1 - i, rect.bottom - 3, rect.right - i), color);

			if (i != 0)
				color = topStripeColors[i - 1];

			surface->FillRect(Rect::Create(rect.top + i, rect.left + 3, rect.top + i + 1, rect.right - 3), color);

			if (i != 0)
				color = bottomStripeColors[i - 1];

			surface->FillRect(Rect::Create(rect.bottom - 1 - i, rect.left + 3, rect.bottom - i, rect.right - 3), color);
		}

		ResolveCachingColor centerCacheColor = centerColor;
		surface->FillRect(rect.Inset(3, 3), centerCacheColor);

		ResolveCachingColor textCacheColor = textColor;

		PortabilityLayer::RenderedFont *sysFont = GetFont(FontPresets::kSystem12Bold);

		int32_t x = (m_rect.left + m_rect.right - static_cast<int32_t>(sysFont->MeasureString(reinterpret_cast<const uint8_t*>(m_text.UnsafeCharPtr()), m_text.Length()))) / 2;
		int32_t y = (m_rect.top + m_rect.bottom + static_cast<int32_t>(sysFont->GetMetrics().m_ascent)) / 2;
		surface->DrawString(Point::Create(x, y), m_text.ToShortStr(), textCacheColor, sysFont);
	}

	void ButtonWidget::DrawAsCheck(DrawSurface *surface, bool inverted)
	{
		if (!m_rect.IsValid())
			return;

		ResolveCachingColor whiteColor = StdColors::White();
		surface->FillRect(m_rect, whiteColor);

		uint16_t checkFrameSize = std::min<uint16_t>(12, std::min(m_rect.Width(), m_rect.Height()));
		int16_t top = (m_rect.top + m_rect.bottom - static_cast<int16_t>(checkFrameSize)) / 2;

		const Rect checkRect = Rect::Create(top, m_rect.left, top + static_cast<int16_t>(checkFrameSize), m_rect.left + static_cast<int16_t>(checkFrameSize));

		ResolveCachingColor *checkColor = nullptr;
		ResolveCachingColor *checkEraseColor = nullptr;
		ResolveCachingColor *textColor = nullptr;

		ResolveCachingColor midGrayColor = RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);
		ResolveCachingColor blackColor = StdColors::Black();

		if (!m_enabled)
		{
			surface->FillRect(checkRect, midGrayColor);
			surface->FillRect(checkRect.Inset(1, 1), lightGrayColor);

			checkColor = &midGrayColor;
			checkEraseColor = &lightGrayColor;
			textColor = &midGrayColor;
		}
		else if (inverted)
		{
			surface->FillRect(checkRect, blackColor);
			surface->FillRect(checkRect.Inset(1, 1), darkGrayColor);

			checkColor = &whiteColor;
			checkEraseColor = &darkGrayColor;
			textColor = &blackColor;
		}
		else
		{
			surface->FillRect(checkRect, blackColor);
			surface->FillRect(checkRect.Inset(1, 1), midGrayColor);

			surface->FillRect(Rect::Create(checkRect.top + 2, checkRect.right - 2, checkRect.bottom - 2, checkRect.right - 1), darkGrayColor);
			surface->FillRect(Rect::Create(checkRect.bottom - 2, checkRect.left + 2, checkRect.bottom - 1, checkRect.right - 1), darkGrayColor);

			surface->FillRect(Rect::Create(checkRect.top + 1, checkRect.left + 1, checkRect.top + 2, checkRect.right - 2), whiteColor);
			surface->FillRect(Rect::Create(checkRect.top + 2, checkRect.left + 1, checkRect.bottom - 2, checkRect.left + 2), whiteColor);

			checkColor = &blackColor;
			checkEraseColor = &midGrayColor;
			textColor = &blackColor;
		}

		if (m_state)
		{
			const Rect checkmarkRect = checkRect.Inset(3, 3);

			if (checkmarkRect.IsValid())
			{
				surface->FillRect(checkmarkRect, *checkColor);

				if (checkmarkRect.Width() >= 5)
				{
					int32_t eraseSpan = checkmarkRect.Width() - 4;
					int16_t coordinateOffset = 0;

					while (eraseSpan > 0)
					{
						surface->FillRect(Rect::Create(checkmarkRect.top + coordinateOffset, checkmarkRect.left + 2 + coordinateOffset, checkmarkRect.top + 1 + coordinateOffset, checkmarkRect.right - 2 - coordinateOffset), *checkEraseColor);
						surface->FillRect(Rect::Create(checkmarkRect.top + 2 + coordinateOffset, checkmarkRect.left + coordinateOffset, checkmarkRect.bottom - 2 - coordinateOffset, checkmarkRect.left + 1 + coordinateOffset), *checkEraseColor);
						surface->FillRect(Rect::Create(checkmarkRect.bottom - 1 - coordinateOffset, checkmarkRect.left + 2 + coordinateOffset, checkmarkRect.bottom - coordinateOffset, checkmarkRect.right - 2 - coordinateOffset), *checkEraseColor);
						surface->FillRect(Rect::Create(checkmarkRect.top + 2 + coordinateOffset, checkmarkRect.right - 1 - coordinateOffset, checkmarkRect.bottom - 2 - coordinateOffset, checkmarkRect.right - coordinateOffset), *checkEraseColor);

						eraseSpan -= 2;
						coordinateOffset++;
					}
				}
			}
		}

		PortabilityLayer::RenderedFont *sysFont = GetFont(FontPresets::kSystem12Bold);
		int32_t textV = (m_rect.top + m_rect.bottom + sysFont->GetMetrics().m_ascent) / 2;
		surface->DrawString(Point::Create(m_rect.left + checkFrameSize + 2, textV), m_text.ToShortStr(), *textColor, sysFont);
	}


	void ButtonWidget::DrawAsRadio(DrawSurface *surface, bool inverted)
	{
		if (!m_rect.IsValid())
			return;

		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor blackColor = StdColors::Black();
		surface->FillRect(m_rect, whiteColor);

		uint16_t checkFrameSize = std::min<uint16_t>(12, std::min(m_rect.Width(), m_rect.Height()));
		int16_t top = (m_rect.top + m_rect.bottom - static_cast<int16_t>(checkFrameSize)) / 2;

		const Rect checkRect = Rect::Create(top, m_rect.left, top + static_cast<int16_t>(checkFrameSize), m_rect.left + static_cast<int16_t>(checkFrameSize));

		ResolveCachingColor *radioColor = nullptr;
		ResolveCachingColor *textColor = nullptr;

		ResolveCachingColor midGrayColor = RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255);
		ResolveCachingColor lightGrayColor = RGBAColor::Create(kButtonLightGray, kButtonLightGray, kButtonLightGray, 255);
		ResolveCachingColor darkGrayColor = RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255);

		if (!m_enabled)
		{
			surface->FillEllipse(checkRect, midGrayColor);
			surface->FillEllipse(checkRect.Inset(1, 1), lightGrayColor);

			radioColor = &midGrayColor;
			textColor = &midGrayColor;
		}
		else if (inverted)
		{
			surface->FillEllipse(checkRect, blackColor);
			surface->FillEllipse(checkRect.Inset(1, 1), darkGrayColor);

			radioColor = &blackColor;
			textColor = &blackColor;
		}
		else
		{
			gs_buttonRadioGraphic.DrawToPixMapWithMask(surface->m_port.GetPixMap(), gs_buttonRadioGraphicMask, checkRect.left, checkRect.top);

			radioColor = &blackColor;
			textColor = &blackColor;
		}

		if (m_state)
		{
			const Rect checkmarkRect = checkRect.Inset(3, 3);

			if (checkmarkRect.IsValid())
			{
				surface->FillEllipse(checkmarkRect, *radioColor);
			}
		}

		PortabilityLayer::RenderedFont *sysFont = GetFont(FontPresets::kSystem12Bold);
		int32_t textV = (m_rect.top + m_rect.bottom + sysFont->GetMetrics().m_ascent) / 2;
		surface->DrawString(Point::Create(m_rect.left + checkFrameSize + 2, textV), m_text.ToShortStr(), *textColor, sysFont);
	}

	void ButtonWidget::DrawDefaultButtonChrome(const Rect &rectRef, DrawSurface *surface)
	{
		const Rect rect = rectRef;
		PixMap **pixMap = surface->m_port.GetPixMap();

		gs_buttonDefaultCornerGraphics[0].DrawToPixMapWithMask(pixMap, gs_buttonDefaultTopLeftGraphicMask, rect.left - 3, rect.top - 3);
		gs_buttonDefaultCornerGraphics[1].DrawToPixMapWithMask(pixMap, gs_buttonDefaultTopRightGraphicMask, rect.right - 2, rect.top - 3);
		gs_buttonDefaultCornerGraphics[2].DrawToPixMapWithMask(pixMap, gs_buttonDefaultBottomLeftGraphicMask, rect.left - 3, rect.bottom - 2);
		gs_buttonDefaultCornerGraphics[3].DrawToPixMapWithMask(pixMap, gs_buttonDefaultBottomRightGraphicMask, rect.right - 2, rect.bottom - 2);

		RGBAColor upperLeftStripeColors[3] =
		{
			RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255),
			StdColors::White(),
			StdColors::Black(),
		};

		RGBAColor bottomRightStripeColors[3] =
		{
			RGBAColor::Create(kButtonMidGray, kButtonMidGray, kButtonMidGray, 255),
			RGBAColor::Create(kButtonDarkGray, kButtonDarkGray, kButtonDarkGray, 255),
			StdColors::Black(),
		};


		for (int i = 0; i < 3; i++)
		{
			ResolveCachingColor color = upperLeftStripeColors[i];
			surface->FillRect(Rect::Create(rect.top - 1 - i, rect.left + 2, rect.top - i, rect.right - 2), color);
			surface->FillRect(Rect::Create(rect.top + 2, rect.left - 1 - i, rect.bottom - 2, rect.left - i), color);
		}

		for (int i = 0; i < 3; i++)
		{
			ResolveCachingColor color = bottomRightStripeColors[i];
			surface->FillRect(Rect::Create(rect.bottom + i, rect.left + 2, rect.bottom + i + 1, rect.right - 2), color);
			surface->FillRect(Rect::Create(rect.top + 2, rect.right + i, rect.bottom - 2, rect.right + i + 1), color);
		}
	}

	void ButtonWidget::SetHighlightStyle(int16_t style, bool enabled)
	{
		if (style == kControlButtonPart)
		{
			if (m_haveHighlightOverride != enabled)
			{
				m_haveHighlightOverride = enabled;
				DrawControl(m_window->GetDrawSurface());
			}
		}
	}
}

PL_IMPLEMENT_WIDGET_TYPE(PortabilityLayer::WidgetTypes::kButton, PortabilityLayer::ButtonWidget)
