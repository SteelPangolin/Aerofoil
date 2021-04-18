#pragma once

#include "CoreDefs.h"
#include <stdint.h>

struct Dialog;
struct DialogTextSubstitutions;
struct Rect;
struct Window;
struct TimeTaggedVOSEvent;
class PLPasStr;

namespace PortabilityLayer
{
	class DialogImpl;
	class Widget;

	class DialogManager
	{
	public:
		virtual Dialog *LoadDialog(int16_t resID, Window *behindWindow, const DialogTextSubstitutions *substitutions) = 0;
		virtual Dialog *LoadDialogFromTemplate(int16_t templateResID, const Rect &rect, bool visible, bool hasCloseBox, uint32_t referenceConstant, uint16_t positionSpec, Window *behindWindow, const PLPasStr &title, const DialogTextSubstitutions *substitutions) = 0;
		GP_ASYNCIFY_PARANOID_VIRTUAL int16_t DisplayAlert(int16_t alertResID, const DialogTextSubstitutions *substitutions) GP_ASYNCIFY_PARANOID_PURE;
		virtual void PositionWindow(Window *window, const Rect &rect) const = 0;

		static DialogManager *GetInstance();
	};

	class DialogItem final
	{
	public:
		explicit DialogItem(Widget *widget);
		~DialogItem();

		Widget *GetWidget() const;

	private:
		friend class DialogImpl;

		Widget *m_widget;
	};
}
