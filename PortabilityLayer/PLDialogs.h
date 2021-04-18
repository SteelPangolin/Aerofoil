#pragma once

#include "PLCore.h"

namespace PortabilityLayer
{
	class DialogItem;
}

template<class T>
class ArrayView;

class PLPasStr;
struct Control;
struct Dialog;

#if GP_ASYNCIFY_PARANOID
namespace DialogFilterFuncs
{
	enum DialogFilterFunc
	{
		kBlowerFilter,
		kFurnitureFilter,
		kCustPictFilter,
		kSwitchFilter,
		kTriggerFilter,
		kApplianceFilter,
		kMicrowaveFilter,
		kGreaseFilter,
		kInvisBonusFilter,
		kTransFilter,
		kEnemyFilter,
		kFlowerFilter,
		kLightFilter,
		kGoToFilter,
		kRoomFilter,
		kOriginalArtFilter,

		kAboutFilter,
		kLicenseReaderFilter,
		kLoadFilter,
		kAboutFrameworkFilter,
		kBrainsFilter,
		kNameFilter,
		kBannerFilter,
		kControlFilter,
		kSoundFilter,
		kHouseFilter,
		kDisplayFilter,
		kPrefsFilter,

		kFileBrowserUIImpl_PopUpAlertUIFilter,
		kFileBrowserUIImpl_FileBrowserUIFilter,
		kDialogManagerImpl_AlertFilter,
	};

	template<int TFilterFuncID>
	int16_t InvokeWrapperThunk(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt);

	template<DialogFilterFunc TFilterFuncID>
	class InvokeWrapper
	{
	public:
		inline int16_t operator()(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt) const
		{
			return InvokeWrapperThunk<TFilterFuncID>(context, dialog, evt);
		}
	};
}

typedef DialogFilterFuncs::DialogFilterFunc DialogFilterFunc_t;

#define PL_FILTER_FUNC(f) DialogFilterFuncs::k##f, DialogFilterFuncs::InvokeWrapper<DialogFilterFuncs::k##f>()
#define PL_IMPLEMENT_FILTER_FUNCTION_THUNK(f, impl)	\
	template<>	\
	int16_t DialogFilterFuncs::InvokeWrapperThunk<DialogFilterFuncs::k##f>(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)	\
	{	\
		return impl(context, dialog, evt);	\
	}

#define PL_IMPLEMENT_FILTER_FUNCTION(f) PL_IMPLEMENT_FILTER_FUNCTION_THUNK(f, f)

#else
typedef int16_t(*DialogFilterFunc_t)(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt);
#define PL_FILTER_FUNC(f) f
#define PL_IMPLEMENT_FILTER_FUNCTION_THUNK(f, impl)
#define PL_IMPLEMENT_FILTER_FUNCTION(f)

#endif

struct DialogTextSubstitutions
{
	Str255 m_strings[4];

	DialogTextSubstitutions();
	explicit DialogTextSubstitutions(const PLPasStr& str0);
	DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1);
	DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1, const PLPasStr& str2);
	DialogTextSubstitutions(const PLPasStr& str0, const PLPasStr& str1, const PLPasStr& str2, const PLPasStr& str3);

private:
	void Init(size_t numItems, const PLPasStr *items);
};

struct Dialog
{
	virtual void Destroy() = 0;

	virtual Window *GetWindow() const = 0;
	virtual ArrayView<const PortabilityLayer::DialogItem> GetItems() const = 0;

	virtual void SetItemVisibility(unsigned int itemIndex, bool isVisible) = 0;

#if GP_ASYNCIFY_PARANOID
	template<class TFilterFunc>
	int16_t ExecuteModal(void *captureContext, DialogFilterFunc_t filterFuncID, const TFilterFunc &filterFunc);
#else
	virtual int16_t ExecuteModal(void *captureContext, DialogFilterFunc_t filterFunc) = 0;
#endif

	virtual bool ReplaceWidget(unsigned int itemIndex, PortabilityLayer::Widget *widget) = 0;
};

typedef Boolean(*ModalFilterUPP)(Dialog *dial, EventRecord *event, short *item);

void DrawDialog(Dialog *dialog);
DrawSurface *GetDialogPort(Dialog *dialog);

void GetDialogItemText(THandle<Control> handle, StringPtr str);

void SetDialogItem(Dialog *dialog, int index, short itemType, THandle<Control> itemHandle, const Rect *itemRect);
void SetDialogItemText(THandle<Control> handle, const PLPasStr &str);

void SelectDialogItemText(Dialog *dialog, int item, int firstSelChar, int lastSelCharExclusive);

void ShowDialogItem(Dialog *dialog, int item);
void HideDialogItem(Dialog *dialog, int item);
