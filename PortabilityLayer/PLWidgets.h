#pragma once

#include "PascalStr.h"
#include "SharedTypes.h"

struct DrawSurface;
class PLPasStr;
struct TimeTaggedVOSEvent;
struct Window;

namespace PortabilityLayer
{
	class Widget;

	namespace WidgetHandleStates
	{
		enum WidgetHandleState
		{
			kIgnored,	// Event was ignored
			kDigested,	// Event was digested by the control
			kCaptured,	// Event was digested by the control and only this control should receive events until it returns a different result
			kActivated,	// Event caused the control to activate
		};
	}

	typedef WidgetHandleStates::WidgetHandleState WidgetHandleState_t;

	typedef void (*WidgetUpdateCallback_t)(void *captureContext, Widget *control, int part);

	struct WidgetBasicState
	{
		WidgetBasicState();

		Rect m_rect;
		PascalStr<255> m_text;
		Window *m_window;
		uint32_t m_refConstant;
		int32_t m_min;
		int32_t m_max;
		int16_t m_state;
		int16_t m_resID;
		bool m_enabled;

		WidgetUpdateCallback_t m_defaultCallback;
	};

	class Widget
	{
	public:
		virtual bool Init(const WidgetBasicState &state, const void *additionalData) = 0;
		virtual void Destroy() = 0;
		virtual WidgetHandleState_t ProcessEvent(void *captureContext, const TimeTaggedVOSEvent &evt);
		virtual int16_t Capture(void *captureContext, const Point &pos, WidgetUpdateCallback_t callback);
		virtual void DrawControl(DrawSurface *surface);

		virtual void SetMin(int32_t v);
		virtual void SetMax(int32_t v);

		void SetPosition(const Point &pos);
		void Resize(uint16_t width, uint16_t height);

		void SetEnabled(bool enabled);
		bool IsEnabled() const;
		virtual void SetState(int16_t state);
		int16_t GetState() const;

		void SetVisible(bool visible);
		bool IsVisible() const;

		virtual void SetString(const PLPasStr &str);
		virtual PLPasStr GetString() const;

		uint32_t GetReferenceConstant() const;

		virtual void SetHighlightStyle(int16_t style, bool enabled);

		virtual bool HandlesTickEvents() const;
		virtual Rect GetExpandedRect() const;

		virtual int ResolvePart(const Point &point) const;

		const Rect &GetRect() const;

		void SetWindow(Window *window);
		Window *GetWindow() const;

		virtual void GainFocus();
		virtual void LoseFocus();

		virtual void OnEnabledChanged();
		virtual void OnStateChanged();
		virtual void OnTick();

	protected:
		friend struct Window;

		explicit Widget(const WidgetBasicState &state);
		virtual ~Widget();

		static void BaseRelease(void *storage);
		static void *BaseAlloc(size_t sz);

		Window *m_window;
		Rect m_rect;
		uint32_t m_referenceConstant;
		int16_t m_state;
		bool m_enabled;
		bool m_visible;
	};
}

#include "PLCore.h"
#include <new>
#include <stdint.h>

namespace PortabilityLayer
{
	template<class T>
	class WidgetSpec : public Widget
	{
	public:
		explicit WidgetSpec(const WidgetBasicState &state)
			: Widget(state)
		{
		}

		void Destroy() override
		{
			this->~WidgetSpec();
			Widget::BaseRelease(static_cast<T*>(this));
		}

		static T *Create(const WidgetBasicState &state, const void *additionalData)
		{
			void *storage = Widget::BaseAlloc(sizeof(T));
			if (!storage)
				return nullptr;

			T *widgetT = new (storage) T(state);

			// Conversion check
			WidgetSpec<T> *downcastWidget = widgetT;
			(void)downcastWidget;

			Widget *widget = widgetT;
			if (!widget->Init(state, additionalData))
			{
				widget->Destroy();
				return nullptr;
			}

			if (state.m_window != nullptr && !static_cast<Window*>(state.m_window)->AddWidget(widget))
			{
				widget->Destroy();
				return nullptr;
			}

			return widgetT;
		}
	};
}
