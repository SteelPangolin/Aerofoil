#include "PLDialogs.h"

#include "DialogManager.h"
#include "PLArrayView.h"
#include "PLPasStr.h"
#include "PLEditboxWidget.h"
#include "PLStandardColors.h"
#include "ResolveCachingColor.h"


DialogTextSubstitutions::DialogTextSubstitutions()
{
	Init(0, nullptr);
}

DialogTextSubstitutions::DialogTextSubstitutions(const PLPasStr& str0)
{
	Init(1, &str0);
}

DialogTextSubstitutions::DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1)
{
	PLPasStr strs[] = { str0, str1 };
	Init(2, strs);
}

DialogTextSubstitutions::DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1, const PLPasStr& str2)
{
	PLPasStr strs[] = { str0, str1, str2 };
	Init(3, strs);
}

DialogTextSubstitutions::DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1, const PLPasStr& str2, const PLPasStr& str3)
{
	PLPasStr strs[] = { str0, str1, str2, str3 };
	Init(4, strs);
}

void DialogTextSubstitutions::Init(size_t numItems, const PLPasStr *items)
{
	for (int i = 0; i < numItems; i++)
	{
		const uint8_t len = items[i].Length();
		m_strings[i][0] = len;
		if (len)
			memcpy(m_strings[i] + 1, items[i].UChars(), len);
	}

	for (size_t i = numItems; i < 4; i++)
		m_strings[i][0] = 0;
}

void DrawDialog(Dialog *dialog)
{
	PL_NotYetImplemented();
}

Dialog *GetNewDialog(int resID, void *unknown, WindowPtr behind)
{
	PL_NotYetImplemented();
	return nullptr;
}

DrawSurface *GetDialogPort(Dialog *dialog)
{
	PL_NotYetImplemented();
	return nullptr;
}

void GetDialogItemText(THandle<Control> handle, StringPtr str)
{
	PL_NotYetImplemented();
}

void SetDialogItem(Dialog *dialog, int index, short itemType, THandle<Control> itemHandle, const Rect *itemRect)
{
	PL_NotYetImplemented();
}

void SetDialogItemText(THandle<Control> handle, const PLPasStr &str)
{
	PL_NotYetImplemented();
}

void SelectDialogItemText(Dialog *dialog, int item, int firstSelChar, int lastSelCharExclusive)
{
	PortabilityLayer::EditboxWidget *widget = static_cast<PortabilityLayer::EditboxWidget*>(dialog->GetItems()[item - 1].GetWidget());
	widget->GetWindow()->FocusWidget(widget);
	widget->SetSelection(firstSelChar, lastSelCharExclusive);
}

void HideDialogItem(Dialog *dialog, int item)
{
	PortabilityLayer::Widget *widget = dialog->GetItems()[item - 1].GetWidget();

	if (widget->IsVisible())
	{
		widget->SetVisible(false);

		DrawSurface *surface = dialog->GetWindow()->GetDrawSurface();

		PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
		surface->FillRect(widget->GetExpandedRect(), whiteColor);
	}
}
