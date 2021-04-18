#include "PLWidgets.h"
#include "MemoryManager.h"
#include "PLControlDefinitions.h"

namespace PortabilityLayer
{
	WidgetBasicState::WidgetBasicState()
		: m_resID(0)
		, m_rect(Rect::Create(0, 0, 0, 0))
		, m_window(nullptr)
		, m_refConstant(0)
		, m_min(0)
		, m_max(0)
		, m_state(0)
		, m_enabled(true)
		, m_defaultCallback(nullptr)
	{
	}

	WidgetHandleState_t Widget::DefaultProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		(void)evt;

		return WidgetHandleStates::kIgnored;
	}

	int16_t Widget::DefaultCapture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return 0;
	}

	void Widget::DrawControl(DrawSurface *surface)
	{
		(void)surface;
	}

#if GP_ASYNCIFY_PARANOID
	WidgetHandleState_t Widget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		switch (this->GetWidgetType())
		{
		case WidgetTypes::kButton:
			return DispatchDynProcessEvent<WidgetTypes::kButton>(this, captureContext, evt);
		case WidgetTypes::kPopupMenu:
			return DispatchDynProcessEvent<WidgetTypes::kPopupMenu>(this, captureContext, evt);
		case WidgetTypes::kEditbox:
			return DispatchDynProcessEvent<WidgetTypes::kEditbox>(this, captureContext, evt);
		case WidgetTypes::kIcon:
			return DispatchDynProcessEvent<WidgetTypes::kIcon>(this, captureContext, evt);
		case WidgetTypes::kImage:
			return DispatchDynProcessEvent<WidgetTypes::kImage>(this, captureContext, evt);
		case WidgetTypes::kInvisible:
			return DispatchDynProcessEvent<WidgetTypes::kInvisible>(this, captureContext, evt);
		case WidgetTypes::kLabel:
			return DispatchDynProcessEvent<WidgetTypes::kLabel>(this, captureContext, evt);
		case WidgetTypes::kScrollBar:
			return DispatchDynProcessEvent<WidgetTypes::kScrollBar>(this, captureContext, evt);
		default:
			return WidgetHandleStates::kIgnored;
		}
	}

	int16_t Widget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		switch (this->GetWidgetType())
		{
		case WidgetTypes::kButton:
			return DispatchDynCapture<WidgetTypes::kButton>(this, captureContext, pos, callback);
		case WidgetTypes::kPopupMenu:
			return DispatchDynCapture<WidgetTypes::kPopupMenu>(this, captureContext, pos, callback);
		case WidgetTypes::kEditbox:
			return DispatchDynCapture<WidgetTypes::kEditbox>(this, captureContext, pos, callback);
		case WidgetTypes::kIcon:
			return DispatchDynCapture<WidgetTypes::kIcon>(this, captureContext, pos, callback);
		case WidgetTypes::kImage:
			return DispatchDynCapture<WidgetTypes::kImage>(this, captureContext, pos, callback);
		case WidgetTypes::kInvisible:
			return DispatchDynCapture<WidgetTypes::kInvisible>(this, captureContext, pos, callback);
		case WidgetTypes::kLabel:
			return DispatchDynCapture<WidgetTypes::kLabel>(this, captureContext, pos, callback);
		case WidgetTypes::kScrollBar:
			return DispatchDynCapture<WidgetTypes::kScrollBar>(this, captureContext, pos, callback);
		default:
			return -1;
		}
	}
#else
	WidgetHandleState_t Widget::ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt)
	{
		return this->DefaultProcessEvent(captureContext, evt);
	}

	int16_t Widget::Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback)
	{
		return this->DefaultCapture(captureContext, pos, callback);
	}
#endif

	void Widget::SetMin(int32_t v)
	{
	}

	void Widget::SetMax(int32_t v)
	{
	}

	void Widget::SetPosition(const Point &pos)
	{
		uint16_t width = m_rect.Width();
		uint16_t height = m_rect.Height();

		m_rect.left = pos.h;
		m_rect.top = pos.v;

		Resize(width, height);
	}

	void Widget::Resize(uint16_t width, uint16_t height)
	{
		m_rect.right = m_rect.left + width;
		m_rect.bottom = m_rect.top + height;
	}

	void Widget::SetEnabled(bool enabled)
	{
		m_enabled = enabled;
		OnEnabledChanged();
	}

	bool Widget::IsEnabled() const
	{
		return m_enabled;
	}

	void Widget::SetState(int16_t state)
	{
		m_state = state;
		OnStateChanged();
	}

	int16_t Widget::GetState() const
	{
		return m_state;
	}

	void Widget::SetVisible(bool visible)
	{
		m_visible = visible;
	}

	bool Widget::IsVisible() const
	{
		return m_visible;
	}

	void Widget::SetHighlightStyle(int16_t style, bool enabled)
	{
		(void)style;
	}

	bool Widget::HandlesTickEvents() const
	{
		return false;
	}

	Rect Widget::GetExpandedRect() const
	{
		return GetRect();
	}

	int Widget::ResolvePart(const Point &point) const
	{
		return kControlButtonPart;
	}

	void Widget::GainFocus()
	{
	}

	void Widget::LoseFocus()
	{
	}

	void Widget::SetString(const PLPasStr &str)
	{
		(void)str;
	}

	PLPasStr Widget::GetString() const
	{
		return PSTR("");
	}

	uint32_t Widget::GetReferenceConstant() const
	{
		return m_referenceConstant;
	}

	const Rect &Widget::GetRect() const
	{
		return m_rect;
	}

	void Widget::SetWindow(Window *window)
	{
		m_window = window;
	}

	Window *Widget::GetWindow() const
	{
		return m_window;
	}

	Widget::Widget(const WidgetBasicState &state)
		: m_rect(state.m_rect)
		, m_window(state.m_window)
		, m_enabled(state.m_enabled)
		, m_referenceConstant(state.m_refConstant)
		, m_state(state.m_state)
		, m_visible(true)
	{
	}

	Widget::~Widget()
	{
	}

	void Widget::OnEnabledChanged()
	{
	}

	void Widget::OnStateChanged()
	{
	}

	void Widget::OnTick()
	{
	}

	void Widget::BaseRelease(void *storage)
	{
		PortabilityLayer::MemoryManager::GetInstance()->Release(storage);
	}

	void *Widget::BaseAlloc(size_t sz)
	{
		return PortabilityLayer::MemoryManager::GetInstance()->Alloc(sz);
	}
}
