#include "DialogManager.h"
#include "IconLoader.h"
#include "IGpDisplayDriver.h"
#include "IGpLogDriver.h"
#include "IGpSystemServices.h"
#include "ResourceManager.h"
#include "QDPixMap.h"
#include "Rect2i.h"
#include "ResTypeID.h"
#include "SharedTypes.h"
#include "UTF8.h"
#include "WindowDef.h"
#include "WindowManager.h"

#include "PLArrayView.h"
#include "PLBigEndian.h"
#include "PLButtonWidget.h"
#include "PLDialogs.h"
#include "PLDrivers.h"
#include "PLEditboxWidget.h"
#include "PLIconWidget.h"
#include "PLImageWidget.h"
#include "PLInvisibleWidget.h"
#include "PLLabelWidget.h"
#include "PLPasStr.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"
#include "PLTimeTaggedVOSEvent.h"
#include "PLWidgets.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include <stdlib.h>
#include <new>

int16_t DialogManagerImpl_AlertFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	return -1;
}

namespace PortabilityLayer
{
	class DialogImpl;
	class Widget;

	namespace SerializedDialogItemTypeCodes
	{
		enum SerializedDialogItemTypeCode
		{
			kUserItem = 0x00,
			kButton = 0x04,
			kCheckBox = 0x05,
			kRadioButton = 0x06,
			kCustomControl = 0x07,
			kLabel = 0x08,
			kEditBox = 0x10,
			kIcon = 0x20,
			kImage = 0x40,
		};
	}

	typedef SerializedDialogItemTypeCodes::SerializedDialogItemTypeCode SerializedDialogItemTypeCode_t;


	struct DialogTemplateItem
	{
		Rect m_rect;
		int16_t m_id;
		uint8_t m_serializedType;
		bool m_enabled;
		Str255 m_name;
	};

	class DialogTemplate final
	{
	public:
		DialogTemplate(DialogTemplateItem *itemStorage, size_t numItems);
		bool DeserializeItems(const rapidjson::Value &itemsArray);
		void Destroy();

		ArrayView<const DialogTemplateItem> GetItems() const;

	private:
		DialogTemplateItem *m_items;
		size_t m_numItems;
	};

	class DialogImpl final : public Dialog
	{
	public:
		void Destroy() override;

		Window *GetWindow() const override;
		ArrayView<const DialogItem> GetItems() const override;
		void SetItemVisibility(unsigned int itemIndex, bool isVisible) override;

#if GP_ASYNCIFY_PARANOID
		template<class TFilterFunc>
		int16_t ExecuteModal(void *captureContext, const TFilterFunc &filterFunc);
#else
		int16_t ExecuteModal(void *captureContext, DialogFilterFunc_t filterFunc) override;
#endif

		bool ReplaceWidget(unsigned int itemIndex, Widget *widget) override;

		bool Populate(DialogTemplate *tmpl, const DialogTextSubstitutions *substitutions);

		void DrawControls(bool redraw);

		Point MouseToDialog(const GpMouseInputEvent &evt);

		static DialogImpl *Create(Window *window, size_t numItems);

	private:
		explicit DialogImpl(Window *window, DialogItem *items, size_t numItems);
		~DialogImpl();

		static void MakeStringSubstitutions(uint8_t *outStr, const uint8_t *inStr, const DialogTextSubstitutions *substitutions);

#if GP_ASYNCIFY_PARANOID
		template<class TFilterFunc>
		int16_t ExecuteModalInDarkenStack(void *captureContext, const TFilterFunc &filterFunc);
#else
		int16_t ExecuteModalInDarkenStack(void *captureContext, DialogFilterFunc_t filterFunc);
#endif

		Window *m_window;
		DialogItem *m_items;
		size_t m_numItems;
		size_t m_maxItems;
	};
}


#if GP_ASYNCIFY_PARANOID

#define PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(f)	\
template<>	\
int16_t Dialog::ExecuteModal(void *captureContext, DialogFilterFunc_t filterFuncID, const DialogFilterFuncs::InvokeWrapper<DialogFilterFuncs::k##f> &filterFunc)	\
{	\
	(void)filterFuncID;	\
	return static_cast<PortabilityLayer::DialogImpl*>(this)->ExecuteModal(captureContext, filterFunc);	\
}

PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(BlowerFilter)

PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(FurnitureFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(CustPictFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(SwitchFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(TriggerFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(ApplianceFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(MicrowaveFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(GreaseFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(InvisBonusFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(TransFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(EnemyFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(FlowerFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(LightFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(GoToFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(RoomFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(OriginalArtFilter)

PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(AboutFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(LicenseReaderFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(LoadFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(AboutFrameworkFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(BrainsFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(NameFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(BannerFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(ControlFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(SoundFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(HouseFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(DisplayFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(PrefsFilter)

PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(FileBrowserUIImpl_PopUpAlertUIFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(FileBrowserUIImpl_FileBrowserUIFilter)
PL_DIALOG_IMPLEMENT_EXECUTE_MODAL(DialogManagerImpl_AlertFilter)

#endif


namespace PortabilityLayer
{
	DialogItem::DialogItem(Widget *widget)
		: m_widget(widget)
	{
	}

	DialogItem::~DialogItem()
	{
	}

	Widget *DialogItem::GetWidget() const
	{
		return m_widget;
	}

	DialogTemplate::DialogTemplate(DialogTemplateItem *itemStorage, size_t numItems)
		: m_items(itemStorage)
		, m_numItems(numItems)
	{
	}

	bool DialogTemplate::DeserializeItems(const rapidjson::Value &itemsArray)
	{
		for (size_t i = 0; i < m_numItems; i++)
		{
			const rapidjson::Value &itemData = itemsArray[static_cast<rapidjson::SizeType>(i)];

			if (!itemData.IsObject())
				return false;

			DialogTemplateItem &item = m_items[i];

			const rapidjson::Value &nameValue = itemData["name"];
			if (nameValue.IsString())
			{
				size_t strSize;
				if (UTF8Processor::DecodeToMacRomanPascalStr(reinterpret_cast<const uint8_t*>(nameValue.GetString()), nameValue.GetStringLength(), item.m_name + 1, sizeof(item.m_name) - 1, strSize))
					item.m_name[0] = static_cast<uint8_t>(strSize);
				else
					item.m_name[0] = 0;
			}
			else if (nameValue.IsArray())
			{
				uint8_t *destName = item.m_name;
				size_t nameLength = nameValue.GetArray().Size();
				if (nameLength > 255)
					nameLength = 255;

				destName[0] = static_cast<uint8_t>(nameLength);

				for (size_t chi = 0; chi < nameLength; chi++)
				{
					const rapidjson::Value &charValue = nameValue.GetArray()[static_cast<rapidjson::SizeType>(chi)];
					if (!charValue.IsUint())
						return false;
					unsigned int charIntValue = charValue.GetUint();
					if (charIntValue >= 256)
						return false;

					destName[1 + chi] = static_cast<uint8_t>(charIntValue);
				}
			}
			else
				return false;

			int32_t posX = 0;
			int32_t posY = 0;

			const rapidjson::Value &posValue = itemData["pos"];
			if (posValue.IsArray())
			{
				if (posValue.GetArray().Size() != 2)
					return false;

				const rapidjson::Value &posXValue = posValue.GetArray()[0];
				const rapidjson::Value &posYValue = posValue.GetArray()[1];

				if (!posXValue.IsInt())
					return false;

				if (!posYValue.IsInt())
					return false;

				posX = posXValue.GetInt();
				posY = posYValue.GetInt();
			}
			else
				return false;

			int32_t sizeX = 0;
			int32_t sizeY = 0;

			const rapidjson::Value &sizeValue = itemData["size"];
			if (sizeValue.IsArray())
			{
				if (sizeValue.GetArray().Size() != 2)
					return false;

				const rapidjson::Value &sizeXValue = sizeValue.GetArray()[0];
				const rapidjson::Value &sizeYValue = sizeValue.GetArray()[1];

				if (!sizeXValue.IsInt())
					return false;

				if (!sizeYValue.IsInt())
					return false;

				sizeX = sizeXValue.GetInt();
				sizeY = sizeYValue.GetInt();
			}
			else
				return false;

			if (posX < INT16_MIN || posX > INT16_MAX || posY < INT16_MIN || posY > INT16_MAX)
				return false;

			if (sizeX < 0 || sizeX >= UINT16_MAX || sizeY < 0 || sizeY >= UINT16_MAX)
				return false;

			const int32_t right = posX + sizeX;
			const int32_t bottom = posY + sizeY;

			if (right > INT16_MAX || bottom > INT16_MAX)
				return false;

			item.m_rect = Rect::Create(posY, posX, bottom, right);

			const rapidjson::Value &itemTypeValue = itemData["itemType"];
			if (itemTypeValue.IsString())
			{
				if (!strcmp(itemTypeValue.GetString(), "UserItem"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kUserItem;
				else if (!strcmp(itemTypeValue.GetString(), "Button"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kButton;
				else if (!strcmp(itemTypeValue.GetString(), "CheckBox"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kCheckBox;
				else if (!strcmp(itemTypeValue.GetString(), "RadioButton"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kRadioButton;
				else if (!strcmp(itemTypeValue.GetString(), "CustomControl"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kCustomControl;
				else if (!strcmp(itemTypeValue.GetString(), "Label"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kLabel;
				else if (!strcmp(itemTypeValue.GetString(), "EditBox"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kEditBox;
				else if (!strcmp(itemTypeValue.GetString(), "Icon"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kIcon;
				else if (!strcmp(itemTypeValue.GetString(), "Image"))
					item.m_serializedType = SerializedDialogItemTypeCodes::kImage;
				else
					return false;
			}
			else
				return false;

			const rapidjson::Value &idValue = itemData["id"];
			if (idValue.IsInt())
			{
				int idInt = idValue.GetInt();
				if (idInt < INT16_MIN || idInt > INT16_MAX)
					return false;

				item.m_id = static_cast<int16_t>(idInt);
			}
			else
				return false;

			const rapidjson::Value &enabledValue = itemData["enabled"];
			if (enabledValue.IsBool())
				item.m_enabled = enabledValue.GetBool();
			else
				return false;
		}

		return true;
	}

	void DialogTemplate::Destroy()
	{
		this->~DialogTemplate();
		free(this);
	}

	ArrayView<const DialogTemplateItem> DialogTemplate::GetItems() const
	{
		return ArrayView<const DialogTemplateItem>(m_items, m_numItems);
	}

	void DialogImpl::Destroy()
	{
		PortabilityLayer::WindowManager::GetInstance()->DestroyWindow(m_window);

		this->~DialogImpl();
		free(this);
	}

	Window *DialogImpl::GetWindow() const
	{
		return m_window;
	}

	ArrayView<const DialogItem> DialogImpl::GetItems() const
	{
		return ArrayView<const DialogItem>(m_items, m_numItems);
	}

	void DialogImpl::SetItemVisibility(unsigned int itemIndex, bool isVisible)
	{
		Widget *widget = m_items[itemIndex].GetWidget();

		if (widget->IsVisible() != isVisible)
		{
			widget->SetVisible(isVisible);

			DrawSurface *surface = m_window->GetDrawSurface();

			if (isVisible)
				widget->DrawControl(surface);
		}
	}

#if GP_ASYNCIFY_PARANOID
	template<class TFilterFunc>
	int16_t DialogImpl::ExecuteModal(void *captureContext, const TFilterFunc &filterFunc)
#else
	int16_t DialogImpl::ExecuteModal(void *captureContext, DialogFilterFunc_t filterFunc)
#endif
	{
		Window *exclWindow = this->GetWindow();

		WindowManager::GetInstance()->SwapExclusiveWindow(exclWindow);

		int16_t result = ExecuteModalInDarkenStack(captureContext, filterFunc);

		WindowManager::GetInstance()->SwapExclusiveWindow(exclWindow);

		return result;
	}

#if GP_ASYNCIFY_PARANOID
	template<class TFilterFunc>
	int16_t DialogImpl::ExecuteModalInDarkenStack(void *captureContext, const TFilterFunc &filterFunc)
#else
	int16_t DialogImpl::ExecuteModalInDarkenStack(void *captureContext, DialogFilterFunc_t filterFunc)
#endif
	{
		Window *window = this->GetWindow();
		Widget *capturingWidget = nullptr;
		size_t capturingWidgetIndex = 0;

		for (;;)
		{
			TimeTaggedVOSEvent evt;

			bool haveEvent = false;
			{
				PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
				haveEvent = WaitForEvent(&evt, 1);
			}

			if (!haveEvent && window->IsHandlingTickEvents())
				window->OnTick();

			const int16_t selection = filterFunc(captureContext, this, haveEvent ? &evt : nullptr);

			if (selection >= 0)
				return selection;

			if (haveEvent)
			{
				if (capturingWidget != nullptr)
				{
					const WidgetHandleState_t state = capturingWidget->ProcessEvent(captureContext, evt);

					if (state != WidgetHandleStates::kDigested)
						capturingWidget = nullptr;

					if (state == WidgetHandleStates::kActivated)
						return static_cast<int16_t>(capturingWidgetIndex + 1);
				}
				else
				{
					if (evt.IsLMouseDownEvent())
					{
						const GpMouseInputEvent &mouseEvent = evt.m_vosEvent.m_event.m_mouseInputEvent;

						Rect2i windowFullRect = WindowManager::GetInstance()->GetWindowFullRect(window);
						if (!windowFullRect.Contains(Vec2i(mouseEvent.m_x, mouseEvent.m_y)))
						{
							PLDrivers::GetSystemServices()->Beep();
							continue;
						}
					}

					const size_t numItems = this->m_numItems;
					for (size_t i = 0; i < numItems; i++)
					{
						Widget *widget = this->m_items[i].GetWidget();

						const WidgetHandleState_t state = widget->ProcessEvent(captureContext, evt);

						if (state == WidgetHandleStates::kActivated)
							return static_cast<int16_t>(i + 1);

						if (state == WidgetHandleStates::kCaptured)
						{
							capturingWidget = widget;
							capturingWidgetIndex = i;
							break;
						}

						if (state == WidgetHandleStates::kDigested)
							break;
					}
				}
			}
		}
	}

	bool DialogImpl::ReplaceWidget(unsigned int itemIndex, Widget *widget)
	{
		DialogItem &item = m_items[itemIndex];
		Widget *oldWidget = item.GetWidget();

		if (!m_window->ReplaceWidget(oldWidget, widget))
			return false;

		m_items[itemIndex].m_widget = widget;

		return true;
	}

	bool DialogImpl::Populate(DialogTemplate *tmpl, const DialogTextSubstitutions *substitutions)
	{
		Window *window = this->GetWindow();

		ArrayView<const DialogTemplateItem> templateItems = tmpl->GetItems();

		const size_t numItems = templateItems.Count();

		for (size_t i = 0; i < numItems; i++)
		{
			const DialogTemplateItem &templateItem = templateItems[i];

			Widget *widget = nullptr;

			Str255 substitutedStr;
			MakeStringSubstitutions(substitutedStr, templateItem.m_name, substitutions);

			WidgetBasicState basicState;
			basicState.m_enabled = templateItem.m_enabled;
			basicState.m_resID = templateItem.m_id;
			basicState.m_text = PascalStr<255>(PLPasStr(substitutedStr));
			basicState.m_rect = templateItem.m_rect;
			basicState.m_window = window;

			switch (templateItem.m_serializedType)
			{
			case SerializedDialogItemTypeCodes::kButton:
				{
					ButtonWidget::AdditionalData addlData;
					addlData.m_buttonStyle = ButtonWidget::kButtonStyle_Button;
					widget = ButtonWidget::Create(basicState, &addlData);
				}
				break;
			case SerializedDialogItemTypeCodes::kLabel:
				widget = LabelWidget::Create(basicState, nullptr);
				break;
			case SerializedDialogItemTypeCodes::kIcon:
				widget = IconWidget::Create(basicState, nullptr);
				break;
			case SerializedDialogItemTypeCodes::kImage:
				widget = ImageWidget::Create(basicState, nullptr);
				break;
			case SerializedDialogItemTypeCodes::kCheckBox:
				{
					ButtonWidget::AdditionalData addlData;
					addlData.m_buttonStyle = ButtonWidget::kButtonStyle_CheckBox;
					widget = ButtonWidget::Create(basicState, &addlData);
				}
				break;
			case SerializedDialogItemTypeCodes::kRadioButton:
				{
					ButtonWidget::AdditionalData addlData;
					addlData.m_buttonStyle = ButtonWidget::kButtonStyle_Radio;
					widget = ButtonWidget::Create(basicState, &addlData);
				}
				break;
			case SerializedDialogItemTypeCodes::kEditBox:
				widget = EditboxWidget::Create(basicState, nullptr);
				break;
			default:
				widget = InvisibleWidget::Create(basicState, nullptr);
				break;
			}

			if (!widget)
				return false;

			new (&m_items[m_numItems++]) DialogItem(widget);
		}

		return true;
	}

	void DialogImpl::DrawControls(bool redraw)
	{
		m_window->DrawControls();
	}

	Point DialogImpl::MouseToDialog(const GpMouseInputEvent &evt)
	{
		return Point::Create(evt.m_x, evt.m_y) - m_window->GetTopLeftCoord();
	}

	void DialogImpl::MakeStringSubstitutions(uint8_t *outStr, const uint8_t *inStr, const DialogTextSubstitutions *substitutions)
	{
		if (substitutions == nullptr)
		{
			memcpy(outStr, inStr, inStr[0] + 1);
			return;
		}

		const uint8_t inStrLen = inStr[0];
		const uint8_t *inStrChar = inStr + 1;

		uint8_t *outStrChar = outStr + 1;

		uint8_t outStrRemaining = 255;
		uint8_t inStrRemaining = inStr[0];
		while (outStrRemaining > 0 && inStrRemaining > 0)
		{
			if ((*inStrChar) != '^' || inStrRemaining < 2 || inStrChar[1] < static_cast<uint8_t>('0') || inStrChar[1] > static_cast<uint8_t>('3'))
			{
				*outStrChar++ = *inStrChar++;
				inStrRemaining--;
				outStrRemaining--;
			}
			else
			{
				const int subIndex = inStrChar[1] - '0';
				inStrChar += 2;
				inStrRemaining -= 2;

				const uint8_t *substitution = substitutions->m_strings[subIndex];
				const uint8_t substitutionLength = substitution[0];
				const uint8_t *substitutionChars = substitution + 1;

				const uint8_t copyLength = (substitutionLength < outStrRemaining) ? substitutionLength : outStrRemaining;
				memcpy(outStrChar, substitutionChars, copyLength);
				outStrChar += copyLength;
				outStrRemaining -= copyLength;
			}
		}

		outStr[0] = static_cast<uint8_t>(outStrChar - (outStr + 1));
	}

	DialogImpl *DialogImpl::Create(Window *window, size_t numItems)
	{
		size_t alignedSize = sizeof(DialogImpl) + GP_SYSTEM_MEMORY_ALIGNMENT + 1;
		alignedSize -= alignedSize % GP_SYSTEM_MEMORY_ALIGNMENT;

		const size_t itemsSize = sizeof(DialogItem) * numItems;

		void *storage = malloc(alignedSize + itemsSize);
		if (!storage)
			return nullptr;

		DialogItem *itemsList = reinterpret_cast<DialogItem *>(static_cast<uint8_t*>(storage) + alignedSize);

		return new (storage) DialogImpl(window, itemsList, numItems);
	}

	DialogImpl::DialogImpl(Window *window, DialogItem *itemsList, size_t numItems)
		: m_window(window)
		, m_items(itemsList)
		, m_numItems(0)
		, m_maxItems(numItems)
	{
	}

	DialogImpl::~DialogImpl()
	{
		while (m_numItems > 0)
		{
			m_numItems--;
			m_items[m_numItems].~DialogItem();
		}
	}

	// DLOG resource format:
	// DialogResourceDataHeader
	// Variable-length PStr: Title
	// Optional: Positioning (2 byte mask)

	struct DialogResourceDataHeader
	{
		BERect m_rect;
		BEInt16_t m_style;
		uint8_t m_visible;
		uint8_t m_unusedA;
		uint8_t m_hasCloseBox;
		uint8_t m_unusedB;
		BEUInt32_t m_referenceConstant;
		BEInt16_t m_itemsResID;
	};

	class DialogManagerImpl final : public DialogManager
	{
	public:
		Dialog *LoadDialog(int16_t resID, Window *behindWindow, const DialogTextSubstitutions *substitutions) override;
		Dialog *LoadDialogFromTemplate(int16_t templateResID, const Rect &rect, bool visible, bool hasCloseBox, uint32_t referenceConstant, uint16_t positionSpec, Window *behindWindow, const PLPasStr &title, const DialogTextSubstitutions *substitutions) override;
		int16_t DisplayAlert(int16_t alertResID, const DialogTextSubstitutions *substitutions) GP_ASYNCIFY_PARANOID_OVERRIDE;
		void PositionWindow(Window *window, const Rect &rect) const override;

		DialogTemplate *LoadDialogTemplate(int16_t resID);

		static DialogManagerImpl *GetInstance();

	private:
		static DialogManagerImpl ms_instance;
	};

	Dialog *DialogManagerImpl::LoadDialog(int16_t resID, Window *behindWindow, const DialogTextSubstitutions *substitutions)
	{
		ResourceManager *rm = ResourceManager::GetInstance();

		THandle<uint8_t> dlogH = rm->GetAppResource('DLOG', resID).StaticCast<uint8_t>();
		const uint8_t *dlogData = *dlogH;
		const uint8_t *dlogDataEnd = dlogData + dlogH.MMBlock()->m_size;

		DialogResourceDataHeader header;
		memcpy(&header, dlogData, sizeof(header));

		const uint8_t *titlePStr = dlogData + sizeof(header);
		const uint8_t *positioningData = titlePStr + 1 + titlePStr[0];	// May be OOB

		BEUInt16_t positionSpec(0);
		if (positioningData != dlogDataEnd)
			memcpy(&positionSpec, positioningData, 2);

		const Rect rect = header.m_rect.ToRect();
		const int16_t style = header.m_style;

		Dialog *dialog = LoadDialogFromTemplate(header.m_itemsResID, rect, header.m_visible != 0, header.m_hasCloseBox != 0, header.m_referenceConstant, positionSpec, behindWindow, PLPasStr(titlePStr), substitutions);

		dlogH.Dispose();

		return dialog;
	}

	Dialog *DialogManagerImpl::LoadDialogFromTemplate(int16_t templateResID, const Rect &rect, bool visible, bool hasCloseBox, uint32_t referenceConstant, uint16_t positionSpec, Window *behindWindow, const PLPasStr &title, const DialogTextSubstitutions *substitutions)
	{
		DialogTemplate *dtemplate = LoadDialogTemplate(templateResID);
		const size_t numItems = (dtemplate == nullptr) ? 0 : dtemplate->GetItems().Count();

		if (!rect.IsValid())
		{
			dtemplate->Destroy();
			return nullptr;
		}

		WindowManager *wm = PortabilityLayer::WindowManager::GetInstance();

		uint16_t styleFlags = WindowStyleFlags::kAlert;
		if (hasCloseBox)
			styleFlags |= PortabilityLayer::WindowStyleFlags::kCloseBox;

		WindowDef wdef = WindowDef::Create(rect, styleFlags, visible, referenceConstant, positionSpec, title);
		Window *window = wm->CreateWindow(wdef);
		if (!window)
		{
			dtemplate->Destroy();
			return nullptr;
		}

		wm->PutWindowBehind(window, behindWindow);

		PositionWindow(window, rect);

		DialogImpl *dialog = DialogImpl::Create(window, numItems);

		if (!dialog)
		{
			dtemplate->Destroy();
			wm->DestroyWindow(window);
			return nullptr;
		}

		if (!dialog->Populate(dtemplate, substitutions))
		{
			dialog->Destroy();
			dtemplate->Destroy();
			wm->DestroyWindow(window);

			return nullptr;
		}

		dialog->DrawControls(true);

		return dialog;
	}

	int16_t DialogManagerImpl::DisplayAlert(int16_t alertResID, const DialogTextSubstitutions *substitutions)
	{
		struct AlertResourceData
		{
			BERect m_rect;
			BEInt16_t m_dialogTemplateResID;

			// Stages are supposed to correspond to how to behave when the alert is displayed multiple times.
			// We just treat all alerts as stage 1.
			BEUInt16_t m_stageData;
		};

		BEUInt16_t autoPosition;
		autoPosition = static_cast<uint16_t>(0);

		THandle<void> alertResource = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('ALRT', alertResID);
		if (!alertResource)
			return 0;

		GP_STATIC_ASSERT(sizeof(AlertResourceData) == 12);

		const size_t resSize = alertResource.MMBlock()->m_rmSelfRef->m_size;
		if (resSize < 12)
		{
			alertResource.Dispose();
			return 0;
		}

		AlertResourceData alertResData;
		memcpy(&alertResData, *alertResource, 12);

		if (resSize == 14)
			memcpy(&autoPosition, static_cast<const uint8_t*>(*alertResource) + 12, 2);

		const uint16_t stageData = alertResData.m_stageData;
		uint8_t boldIndexes[4];
		uint8_t soundIndexes[4];
		bool isVisible[4];
		for (int i = 0; i < 4; i++)
		{
			boldIndexes[i] = static_cast<uint8_t>((stageData >> (i * 4)) & 0x1);
			isVisible[i] = (((stageData >> (i * 4)) & 0x3) != 0);
			soundIndexes[i] = static_cast<uint8_t>((stageData >> (i * 4 + 2)) & 0x1);
		}

		// If sound index is 0, play no sound

		if (soundIndexes[0] != 0)
			PLDrivers::GetSystemServices()->Beep();

		const Rect dialogRect = alertResData.m_rect.ToRect();

		Dialog *dialog = LoadDialogFromTemplate(alertResData.m_dialogTemplateResID, alertResData.m_rect.ToRect(), true, false, 0, 0x300a, PL_GetPutInFrontWindowPtr(), PSTR(""), substitutions);
		if (!dialog)
			return 0;

		int16_t hit = dialog->ExecuteModal(nullptr, PL_FILTER_FUNC(DialogManagerImpl_AlertFilter));
		dialog->Destroy();

		return hit;
	}

	DialogTemplate *DialogManagerImpl::LoadDialogTemplate(int16_t resID)
	{
		ResourceManager *rm = ResourceManager::GetInstance();

		THandle<uint8_t> dtemplateH = rm->GetAppResource('DITL', resID).StaticCast<uint8_t>();

		if (!dtemplateH)
			return nullptr;

		rapidjson::Document document;
		document.Parse(reinterpret_cast<const char*>(*dtemplateH), dtemplateH.MMBlock()->m_size);

		dtemplateH.Dispose();

		if (document.HasParseError())
			return nullptr;

		if (!document.IsObject() || !document.HasMember("items"))
			return nullptr;

		const rapidjson::Value &itemsArray = document["items"];

		if (!itemsArray.IsArray())
			return nullptr;

		const size_t numItems = itemsArray.GetArray().Size();

		if (numItems > 0xffff)
			return nullptr;

		size_t dtlAlignedSize = sizeof(DialogTemplate) + GP_SYSTEM_MEMORY_ALIGNMENT - 1;
		dtlAlignedSize -= dtlAlignedSize % GP_SYSTEM_MEMORY_ALIGNMENT;

		const size_t dtlItemSize = sizeof(DialogTemplateItem) * numItems;

		void *storage = malloc(dtlAlignedSize + dtlItemSize);
		if (!storage)
		{
			dtemplateH.Dispose();
			return nullptr;
		}

		uint8_t *itemsLoc = static_cast<uint8_t*>(storage) + dtlAlignedSize;

		DialogTemplate *dtemplate = new (storage) DialogTemplate(reinterpret_cast<DialogTemplateItem*>(itemsLoc), numItems);
		if (!dtemplate->DeserializeItems(itemsArray))
		{
			dtemplate->Destroy();
			return nullptr;
		}

		return dtemplate;
	}

	void DialogManagerImpl::PositionWindow(Window *window, const Rect &rect) const
	{
		IGpLogDriver *logger = PLDrivers::GetLogDriver();

		Vec2i displayResolution = WindowManager::GetInstance()->GetDisplayResolution();
		unsigned int displayWidth = displayResolution.m_x;
		unsigned int displayHeight = displayResolution.m_y;

		const unsigned int halfDisplayHeight = displayHeight / 2;
		const unsigned int quarterDisplayWidth = displayHeight / 4;
		const unsigned int halfDisplayWidth = displayWidth;

		const uint16_t dialogWidth = rect.Width();
		const uint16_t dialogHeight = rect.Height();

		if (logger)
			logger->Printf(IGpLogDriver::Category_Information, "Auto positioning window size %ix%i on display size %ix%i", static_cast<int>(dialogWidth), static_cast<int>(dialogHeight), static_cast<int>(displayWidth), static_cast<int>(displayHeight));

		Vec2i newPosition;

		newPosition.m_x = (static_cast<int32_t>(displayWidth) - static_cast<int32_t>(dialogWidth)) / 2;

		// We center dialogs vertically in one of 3 ways in this priority:
		// - Centered at 1/3 until the top edge is at the 1/4 mark
		// - Top edge aligned to 1/4 mark until bottom edge is at 3/4 mark
		// - Centered on screen

		//if (displayHeight / 3 - dialogHeight / 2 >= displayHeight / 4)
		if (static_cast<int32_t>(displayHeight * 4) - static_cast<int32_t>(dialogHeight * 6) >= static_cast<int32_t>(displayHeight * 3))
		{
			//newPosition.m_y = displayHeight / 3 - dialogHeight / 2;
			newPosition.m_y = (static_cast<int32_t>(displayHeight * 2) - static_cast<int32_t>(dialogHeight * 3)) / 6;
		}
		else if (dialogHeight * 2U <= displayHeight)
			newPosition.m_y = displayHeight / 4;
		else
			newPosition.m_y = (static_cast<int32_t>(displayHeight) - static_cast<int32_t>(dialogHeight)) / 2;

		if (logger)
			logger->Printf(IGpLogDriver::Category_Information, "Positioned at %i,%i", static_cast<int>(newPosition.m_x), static_cast<int>(newPosition.m_y));

		window->SetPosition(newPosition);
	}

	DialogManagerImpl *DialogManagerImpl::GetInstance()
	{
		return &ms_instance;
	}

	DialogManagerImpl DialogManagerImpl::ms_instance;

	DialogManager *DialogManager::GetInstance()
	{
		return DialogManagerImpl::GetInstance();
	}
}

PL_IMPLEMENT_FILTER_FUNCTION(DialogManagerImpl_AlertFilter)

#if GP_ASYNCIFY_PARANOID
int16_t PortabilityLayer::DialogManager::DisplayAlert(int16_t alertResID, const DialogTextSubstitutions *substitutions)
{
	return static_cast<DialogManagerImpl*>(this)->DisplayAlert(alertResID, substitutions);
}
#endif
