#include "FileBrowserUI.h"

#include "DialogManager.h"
#include "PLButtonWidget.h"
#include "FileManager.h"
#include "FontFamily.h"
#include "GpApplicationName.h"
#include "GpBuildVersion.h"
#include "GpIOStream.h"
#include "GpRenderedFontMetrics.h"
#include "IGpDirectoryCursor.h"
#include "IGpFileSystem.h"
#include "IGpFont.h"
#include "IGpSystemServices.h"
#include "WindowManager.h"
#include "MacFileInfo.h"
#include "MemoryManager.h"
#include "PLStandardColors.h"
#include "RenderedFont.h"
#include "ResolveCachingColor.h"
#include "WindowDef.h"
#include "MacRomanConversion.h"

#include "PLArrayView.h"
#include "PLControlDefinitions.h"
#include "PLCore.h"
#include "PLDialogs.h"
#include "PLDrivers.h"
#include "PLEditboxWidget.h"
#include "PLKeyEncoding.h"
#include "PLQDraw.h"
#include "PLScrollBarWidget.h"
#include "PLSysCalls.h"
#include "PLTimeTaggedVOSEvent.h"

#include <algorithm>

static const int kOkayButton = 1;
static const int kCancelButton = 2;
static const int kFileList = 3;
static const int kFileListScrollBar = 4;
static const int kFileNameEditBox = 5;
static const int kDeleteButton = 5;
static const int kFileBrowserUIOpenDialogTemplateID = 2001;
static const int kFileBrowserUISaveDialogTemplateID = 2002;
static const int kFileBrowserUIOverwriteDialogTemplateID = 2003;
static const int kFileBrowserUIBadNameDialogTemplateID = 2004;
static const int kFileBrowserUISaveDialogUnobstructiveTemplateID = 2007;
static const int kFileBrowserUIDeleteDialogTemplateID = 2008;


static const int kOverwriteNoButton = 1;
static const int kOverwriteYesButton = 2;

namespace PortabilityLayer
{
	class FileBrowserUIImpl
	{
	public:
		explicit FileBrowserUIImpl(const FileBrowserUI_DetailsCallbackAPI &callbackAPI);
		~FileBrowserUIImpl();

		static void PubScrollBarCallback(void *captureContext, Widget *control, int part);
		static bool PubEditBoxCharFilter(void *context, uint8_t ch);

		bool AppendName(const char *name, size_t nameLength, void *details);
		void SortNames();
		void DrawHeaders();
		void DrawFileList();

		void CaptureFileListDrag();
		void SetScrollOffset(int32_t offset);
		uint16_t GetScrollCapacity() const;

		void SetUIComponents(Window *window, DrawSurface *surface, const Rect &fileListRect, EditboxWidget *editBox);

		PLPasStr GetSelectedFileName() const;

		void RemoveSelectedFile();

		static int16_t PopUpAlert(const Rect &rect, int dialogResID, const DialogTextSubstitutions *substitutions);

		int16_t FileBrowserUIFilter(Dialog *dialog, const TimeTaggedVOSEvent *evt);
		int16_t PopUpAlertUIFilter(Dialog *dialog, const TimeTaggedVOSEvent *evt);

	private:
		typedef PascalStr<255> NameStr_t;

		struct FileEntry
		{
			NameStr_t m_nameStr;
			void *m_fileDetails;
		};

		void ScrollBarCallback(Widget *control, int part);

		static bool FileEntrySortPred(const FileEntry &a, const FileEntry &b);

		int m_offset;
		int m_selectedIndex;
		int32_t m_scrollOffset;
		int32_t m_fontSpacing;
		THandle<FileEntry> m_entries;
		size_t m_numEntries;
		DrawSurface *m_surface;
		Window *m_window;
		EditboxWidget *m_editBox;
		Rect m_rect;

		Point m_doubleClickPos;
		uint32_t m_doubleClickTime;
		bool m_haveFirstClick;

		const FileBrowserUI_DetailsCallbackAPI m_api;
	};
}


static int16_t FileBrowserUIImpl_FileBrowserUIFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	return static_cast<PortabilityLayer::FileBrowserUIImpl*>(context)->FileBrowserUIFilter(dialog, evt);
}

static int16_t FileBrowserUIImpl_PopUpAlertUIFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	return static_cast<PortabilityLayer::FileBrowserUIImpl*>(context)->PopUpAlertUIFilter(dialog, evt);
}

namespace PortabilityLayer
{
	FileBrowserUIImpl::FileBrowserUIImpl(const FileBrowserUI_DetailsCallbackAPI &callbackAPI)
		: m_offset(0)
		, m_surface(nullptr)
		, m_window(nullptr)
		, m_editBox(nullptr)
		, m_rect(Rect::Create(0, 0, 0, 0))
		, m_selectedIndex(-1)
		, m_scrollOffset(0)
		, m_fontSpacing(1)
		, m_numEntries(0)
		, m_doubleClickPos(Point::Create(0, 0))
		, m_doubleClickTime(0)
		, m_haveFirstClick(false)
		, m_api(callbackAPI)
	{
	}

	FileBrowserUIImpl::~FileBrowserUIImpl()
	{
		if (m_entries)
		{
			FileEntry *entries = *m_entries;
			for (size_t i = 0; i < m_numEntries; i++)
				m_api.m_freeFileDetailsCallback(entries[i].m_fileDetails);
		}
		m_entries.Dispose();
	}

	void FileBrowserUIImpl::PubScrollBarCallback(void *captureContext, Widget *control, int part)
	{
		static_cast<FileBrowserUIImpl*>(captureContext)->ScrollBarCallback(control, part);
	}

	bool FileBrowserUIImpl::PubEditBoxCharFilter(void *context, uint8_t ch)
	{
		uint16_t unicodeChar = MacRoman::ToUnicode(ch);

		return PLDrivers::GetFileSystem()->ValidateFilePathUnicodeChar(unicodeChar);
	}

	bool FileBrowserUIImpl::AppendName(const char *name, size_t nameLen, void *details)
	{
		MemoryManager *mm = MemoryManager::GetInstance();
		if (!m_entries)
		{
			m_entries = THandle<FileEntry>(mm->AllocHandle(0));
			if (!m_entries)
				return false;
		}

		size_t oldSize = m_entries.MMBlock()->m_size;

		if (!mm->ResizeHandle(m_entries.MMBlock(), oldSize + sizeof(FileEntry)))
			return false;

		FileEntry &entry = (*m_entries)[m_numEntries++];
		entry.m_nameStr = NameStr_t(nameLen, name);
		entry.m_fileDetails = details;

		return true;
	}

	void FileBrowserUIImpl::SortNames()
	{
		if (!m_entries)
			return;

		FileEntry *entries = *m_entries;

		std::sort(entries, entries + m_numEntries, FileEntrySortPred);
	}

	void FileBrowserUIImpl::DrawHeaders()
	{
		PortabilityLayer::RenderedFont *font = GetFont(FontPresets::kApplication12Bold);

		ResolveCachingColor blackColor = StdColors::Black();

		const Point basePoint = Point::Create(16, 16 + font->GetMetrics().m_ascent);
		m_api.m_drawLabelsCallback(m_surface, basePoint);
	}

	void FileBrowserUIImpl::DrawFileList()
	{
		if (!m_entries.MMBlock())
			return;

		PortabilityLayer::RenderedFont *font = GetFont(FontPresets::kApplication12Bold);

		GpRenderedFontMetrics metrics = font->GetMetrics();
		int32_t spacing = metrics.m_linegap;
		int32_t glyphOffset = (metrics.m_linegap + metrics.m_ascent) / 2;

		Rect itemRect = Rect::Create(m_rect.top, m_rect.left, m_rect.top + spacing, m_rect.right);
		itemRect.top -= m_scrollOffset;
		itemRect.bottom -= m_scrollOffset;

		ResolveCachingColor blackColor = StdColors::Black();
		ResolveCachingColor whiteColor = StdColors::White();
		ResolveCachingColor focusColor = RGBAColor::Create(153, 153, 255, 255);

		m_surface->FillRect(m_rect, whiteColor);

		for (size_t i = 0; i < m_numEntries; i++)
		{
			if (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) == i)
			{
				Rect focusRect = itemRect.Intersect(m_rect);
				if (focusRect.IsValid())
					m_surface->FillRect(focusRect, focusColor);
			}

			Point itemStringPoint = Point::Create(itemRect.left + 2, itemRect.top + glyphOffset);
			m_surface->DrawStringConstrained(itemStringPoint, (*m_entries)[i].m_nameStr.ToShortStr(), m_rect, blackColor, font);

			m_api.m_drawFileDetailsCallback(m_surface, itemStringPoint, m_rect, (*m_entries)[i].m_fileDetails);

			itemRect.top += spacing;
			itemRect.bottom += spacing;
		}

		m_fontSpacing = spacing;
	}

	void FileBrowserUIImpl::CaptureFileListDrag()
	{
	}

	void FileBrowserUIImpl::SetScrollOffset(int32_t offset)
	{
		m_scrollOffset = offset;
		DrawFileList();
	}

	uint16_t FileBrowserUIImpl::GetScrollCapacity() const
	{
		int32_t boxHeight = m_rect.Height();
		int32_t overCapacity = (static_cast<int32_t>(m_numEntries) * m_fontSpacing - boxHeight);

		if (overCapacity < 0)
			return 0;
		else
			return static_cast<uint16_t>(overCapacity);
	}

	void FileBrowserUIImpl::SetUIComponents(Window *window, DrawSurface *surface, const Rect &rect, EditboxWidget *editbox)
	{
		m_surface = surface;
		m_rect = rect;
		m_window = window;
		m_editBox = editbox;
	}

	PLPasStr FileBrowserUIImpl::GetSelectedFileName() const
	{
		if (m_selectedIndex < 0)
			return PSTR("");
		else
			return (*m_entries)[m_selectedIndex].m_nameStr.ToShortStr();
	}

	void FileBrowserUIImpl::RemoveSelectedFile()
	{
		if (m_selectedIndex < 0)
			return;


		FileEntry *entries = *m_entries;

		FileEntry &removedEntry = entries[m_selectedIndex];
		m_api.m_freeFileDetailsCallback(removedEntry.m_fileDetails);

		for (size_t i = m_selectedIndex; i < m_numEntries - 1; i++)
			entries[i] = entries[i + 1];

		m_numEntries--;
		PortabilityLayer::MemoryManager::GetInstance()->ResizeHandle(m_entries.MMBlock(), sizeof(FileEntry) * m_numEntries);

		m_selectedIndex = -1;
		DrawFileList();
	}

	void FileBrowserUIImpl::ScrollBarCallback(Widget *control, int part)
	{
		const int pageStepping = 5;

		switch (part)
		{
		case kControlUpButtonPart:
			control->SetState(control->GetState() - m_fontSpacing);
			break;
		case kControlDownButtonPart:
			control->SetState(control->GetState() + m_fontSpacing);
			break;
		case kControlPageUpPart:
			control->SetState(control->GetState() - pageStepping * m_fontSpacing);
			break;
		case kControlPageDownPart:
			control->SetState(control->GetState() + pageStepping * m_fontSpacing);
			break;
		default:
			break;
		};

		SetScrollOffset(control->GetState());
	}

	static FileBrowserUI::Mode gs_currentFileBrowserUIMode;

	int16_t FileBrowserUIImpl::FileBrowserUIFilter(Dialog *dialog, const TimeTaggedVOSEvent *evt)
	{
		bool		handledIt = false;
		int16_t		hit = -1;

		if (!evt)
			return -1;

		Window *window = dialog->GetWindow();
		DrawSurface *surface = window->GetDrawSurface();

		if (evt->IsKeyDownEvent())
		{
			switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
			{
			case PL_KEY_SPECIAL(kEnter):
			case PL_KEY_NUMPAD_SPECIAL(kEnter):
				{
					Widget *okayButton = dialog->GetItems()[kOkayButton - 1].GetWidget();

					if (okayButton->IsEnabled())
					{
						PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
						okayButton->SetHighlightStyle(kControlButtonPart, true);
						PLSysCalls::Sleep(8);
						okayButton->SetHighlightStyle(kControlButtonPart, false);

						hit = kOkayButton;
						handledIt = true;
					}
				}
				break;
				
			case PL_KEY_SPECIAL(kEscape):
				{
					PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
					Widget *cancelButton = dialog->GetItems()[kCancelButton - 1].GetWidget();

					cancelButton->SetHighlightStyle(kControlButtonPart, true);
					PLSysCalls::Sleep(8);
					cancelButton->SetHighlightStyle(kControlButtonPart, false);

					hit = kCancelButton;
					handledIt = true;
				}
				break;

			default:
				handledIt = false;
				break;
			}
		}

		if (evt->IsLMouseDownEvent())
		{
			Point mousePt = m_window->MouseToLocal(evt->m_vosEvent.m_event.m_mouseInputEvent);
			bool haveDoubleClick = false;

			if (m_rect.Contains(mousePt))
			{
				if (m_haveFirstClick)
				{
					const uint32_t doubleTime = 30;	// PL_NotYetImplemented_TODO: Get this from the system settings

					if (mousePt == m_doubleClickPos && evt->m_timestamp - m_doubleClickTime < doubleTime)
						haveDoubleClick = true;
				}

				if (!haveDoubleClick)
				{
					m_haveFirstClick = true;
					m_doubleClickPos = mousePt;
					m_doubleClickTime = evt->m_timestamp;

					const TimeTaggedVOSEvent *rcvEvt = evt;

					TimeTaggedVOSEvent evtHolder;
					for (;;)
					{
						if (rcvEvt)
						{
							if (rcvEvt->m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
							{
								mousePt = m_window->MouseToLocal(rcvEvt->m_vosEvent.m_event.m_mouseInputEvent);
								if (mousePt != m_doubleClickPos)
									m_haveFirstClick = false;

								if (m_rect.Contains(mousePt))
								{
									int32_t selection = (mousePt.v - m_rect.top + m_scrollOffset) / m_fontSpacing;

									if (selection < 0 || static_cast<size_t>(selection) >= m_numEntries)
										selection = -1;

									if (selection >= 0)
									{
										if (selection != m_selectedIndex)
										{
											m_selectedIndex = selection;

											dialog->GetItems()[kOkayButton - 1].GetWidget()->SetEnabled(selection >= 0);

											if (gs_currentFileBrowserUIMode == FileBrowserUI::Mode_Open)
												dialog->GetItems()[kDeleteButton - 1].GetWidget()->SetEnabled(selection >= 0);

											DrawFileList();
										}

										if (m_editBox)
										{
											PLPasStr nameStr = (*m_entries)[m_selectedIndex].m_nameStr.ToShortStr();
											m_editBox->SetString(nameStr);
											m_editBox->SetSelection(0, nameStr.Length());
										}
									}
								}
							}

							if (rcvEvt->IsLMouseUpEvent())
								break;
						}

						if (WaitForEvent(&evtHolder, 1))
							rcvEvt = &evtHolder;
						else
							rcvEvt = nullptr;
					}
				}
			}

			if (haveDoubleClick && m_selectedIndex >= 0)
			{
				handledIt = true;
				hit = kOkayButton;
			}
		}

		if (!handledIt)
			return -1;

		return hit;
	}

	int16_t FileBrowserUIImpl::PopUpAlertUIFilter(Dialog *dialog, const TimeTaggedVOSEvent *evt)
	{
		bool		handledIt = false;
		int16_t		hit = -1;

		if (!evt)
			return -1;

		Window *window = dialog->GetWindow();
		DrawSurface *surface = window->GetDrawSurface();

		if (evt->IsKeyDownEvent())
		{
			switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
			{
				case PL_KEY_SPECIAL(kEnter):
				case PL_KEY_NUMPAD_SPECIAL(kEnter):
				{
					Widget *okayButton = dialog->GetItems()[kOkayButton - 1].GetWidget();

					if (okayButton->IsEnabled())
					{
						okayButton->SetHighlightStyle(kControlButtonPart, true);
						PLSysCalls::Sleep(8);
						okayButton->SetHighlightStyle(kControlButtonPart, false);

						hit = kOkayButton;
						handledIt = true;
					}
				}
				break;

			default:
				handledIt = false;
				break;
			}
		}

		if (!handledIt)
			return -1;

		return hit;
	}

	bool FileBrowserUIImpl::FileEntrySortPred(const FileEntry &a, const FileEntry &b)
	{
		const size_t lenA = a.m_nameStr.Length();
		const size_t lenB = b.m_nameStr.Length();

		const size_t shorterLength = std::min(lenA, lenB);

		int comparison = memcmp(a.m_nameStr.UnsafeCharPtr(), b.m_nameStr.UnsafeCharPtr(), shorterLength);
		if (comparison > 0)
			return false;

		if (comparison < 0)
			return true;

		return lenA < lenB;
	}

	int16_t FileBrowserUIImpl::PopUpAlert(const Rect &rect, int dialogResID, const DialogTextSubstitutions *substitutions)
	{
		IGpSystemServices *sysServices = PLDrivers::GetSystemServices();

		// Disable text input temporarily and restore it after
		const bool wasTextInput = sysServices->IsTextInputEnabled();
		sysServices->SetTextInputEnabled(false);

		PortabilityLayer::DialogManager *dialogManager = PortabilityLayer::DialogManager::GetInstance();
		Dialog *dialog = dialogManager->LoadDialogFromTemplate(dialogResID, rect, true, false, 0, 0, PL_GetPutInFrontWindowPtr(), PSTR(""), substitutions);

		const PortabilityLayer::DialogItem	&firstItem = *dialog->GetItems().begin();
		Rect								itemRect = firstItem.GetWidget()->GetRect();

		PortabilityLayer::ButtonWidget::DrawDefaultButtonChrome(itemRect, dialog->GetWindow()->GetDrawSurface());

		int16_t hit = 0;
		do 
		{
			hit = dialog->ExecuteModal(nullptr, PL_FILTER_FUNC(FileBrowserUIImpl_PopUpAlertUIFilter));
		} while (hit != kOkayButton && hit != kCancelButton);

		dialog->Destroy();

		sysServices->SetTextInputEnabled(wasTextInput);

		return hit;
	}

	bool FileBrowserUI::Prompt(Mode mode, VirtualDirectory_t dirID, const char *extension, char *path, size_t &outPathLength, size_t pathCapacity, const PLPasStr &initialFileName, const PLPasStr &promptText, bool composites, const FileBrowserUI_DetailsCallbackAPI &callbackAPI)
	{
		size_t extensionLength = strlen(extension);

		int dialogID = 0;
		bool isObstructive = false;
		int windowHeight = 272;
		if (mode == Mode_Open)
			dialogID = kFileBrowserUIOpenDialogTemplateID;
		else if (mode == Mode_Save)
		{
			if (PLDrivers::GetSystemServices()->IsTextInputObstructive())
			{
				dialogID = kFileBrowserUISaveDialogUnobstructiveTemplateID;
				windowHeight = 208;
				isObstructive = true;
			}
			else
				dialogID = kFileBrowserUISaveDialogTemplateID;
		}
		else
		{
			assert(false);
			return false;
		}

		FileBrowserUIImpl uiImpl(callbackAPI);

		// Enumerate files
		IGpFileSystem *fs = PLDrivers::GetFileSystem();
		IGpDirectoryCursor *dirCursor = fs->ScanDirectory(dirID);

		if (!dirCursor)
			return false;

		const char *fileName;
		while (dirCursor->GetNext(fileName))
		{
			size_t nameLength = strlen(fileName);

			if (nameLength < extensionLength)
				continue;

			const char *nameExt = fileName + (nameLength - extensionLength);

			if (!memcmp(nameExt, extension, extensionLength))
			{
				PLPasStr fnamePStr = PLPasStr(nameLength - extensionLength, fileName);
				if (!callbackAPI.m_filterFileCallback(dirID, fnamePStr))
					continue;

				if (!uiImpl.AppendName(fileName, nameLength - extensionLength, callbackAPI.m_loadFileDetailsCallback(dirID, fnamePStr)))
				{
					dirCursor->Destroy();
					return false;
				}
			}
		}

		uiImpl.SortNames();

		dirCursor->Destroy();

		const int scrollBarWidth = 16;
		const Rect windowRect = Rect::Create(0, 0, windowHeight, 450);

		PortabilityLayer::WindowDef wdef = PortabilityLayer::WindowDef::Create(windowRect, PortabilityLayer::WindowStyleFlags::kAlert, true, 0, 0, PSTR(""));

		PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
		PortabilityLayer::RenderedFont *font = GetFont(FontPresets::kApplication12Bold);
		PortabilityLayer::RenderedFont *fontLight = GetFont(FontPresets::kApplication8);

		int16_t verticalPoint = 16 + font->GetMetrics().m_ascent;
		int16_t horizontalOffset = 16;
		const int16_t spacing = 12;

		PortabilityLayer::DialogManager *dialogManager = PortabilityLayer::DialogManager::GetInstance();

		DialogTextSubstitutions substitutions(promptText);
		Dialog *dialog = dialogManager->LoadDialogFromTemplate(dialogID, windowRect, true, false, 0, 0, PL_GetPutInFrontWindowPtr(), PSTR(""), &substitutions);

		Window *window = dialog->GetWindow();

		if (isObstructive)
			window->SetPosition(PortabilityLayer::Vec2i(window->GetPosition().m_x, 6));

		DrawSurface *surface = window->GetDrawSurface();

		const PortabilityLayer::DialogItem	&firstItem = *dialog->GetItems().begin();
		Rect								itemRect = firstItem.GetWidget()->GetRect();

		PortabilityLayer::ButtonWidget::DrawDefaultButtonChrome(itemRect, surface);

		// Get item rects
		const Rect fileListRect = dialog->GetItems()[kFileList - 1].GetWidget()->GetRect();
		const Rect scrollBarRect = dialog->GetItems()[kFileListScrollBar - 1].GetWidget()->GetRect();

		EditboxWidget *editbox = nullptr;
		if (mode == Mode_Save)
		{
			editbox = static_cast<EditboxWidget*>(dialog->GetItems()[kFileNameEditBox - 1].GetWidget());
			editbox->SetCharacterFilter(&uiImpl, FileBrowserUIImpl::PubEditBoxCharFilter);
			editbox->SetCapacity(31);
			editbox->SetString(initialFileName);

			dialog->GetWindow()->FocusWidget(editbox);
		}

		// Draw file list frame
		surface->FrameRect(fileListRect.Inset(-1, -1), blackColor);

		// Draw initial stuff
		uiImpl.SetUIComponents(dialog->GetWindow(), surface, fileListRect, editbox);
		uiImpl.DrawFileList();

		PortabilityLayer::ScrollBarWidget *scrollBar = nullptr;

		{
			PortabilityLayer::WidgetBasicState state;
			state.m_rect = scrollBarRect;
			state.m_refConstant = 0;
			state.m_window = nullptr;
			state.m_max = uiImpl.GetScrollCapacity();
			state.m_state = 0;
			state.m_defaultCallback = FileBrowserUIImpl::PubScrollBarCallback;
			scrollBar = PortabilityLayer::ScrollBarWidget::Create(state, nullptr);
		}

		dialog->ReplaceWidget(kFileListScrollBar - 1, scrollBar);

		window->DrawControls();

		uiImpl.DrawHeaders();

		int16_t hit = 0;

		Window *exclWindow = dialog->GetWindow();

		WindowManager::GetInstance()->SwapExclusiveWindow(exclWindow);

		gs_currentFileBrowserUIMode = mode;

		do
		{
			hit = dialog->ExecuteModal(&uiImpl, PL_FILTER_FUNC(FileBrowserUIImpl_FileBrowserUIFilter));

			if (hit == kFileListScrollBar)
				uiImpl.SetScrollOffset(scrollBar->GetState());

			if (hit == kOkayButton && mode == Mode_Save)
			{
				IGpFileSystem *fs = PLDrivers::GetFileSystem();

				EditboxWidget *editBox = static_cast<EditboxWidget*>(dialog->GetItems()[kFileNameEditBox - 1].GetWidget());

				PLPasStr nameStr = editBox->GetString();
				if (nameStr.Length() == 0 || !fs->ValidateFilePath(nameStr.Chars(), nameStr.Length()))
				{
					PLDrivers::GetSystemServices()->Beep();
					FileBrowserUIImpl::PopUpAlert(Rect::Create(0, 0, 135, 327), kFileBrowserUIBadNameDialogTemplateID, nullptr);
					hit = -1;
				}
				else
				{
					bool fileExists = false;
					if (composites)
						fileExists = PortabilityLayer::FileManager::GetInstance()->CompositeFileExists(dirID, nameStr);
					else
						fileExists = PortabilityLayer::FileManager::GetInstance()->NonCompositeFileExists(dirID, nameStr, extension);

					if (fileExists)
					{
						DialogTextSubstitutions substitutions(nameStr);

						PLDrivers::GetSystemServices()->Beep();
						int16_t subHit = FileBrowserUIImpl::PopUpAlert(Rect::Create(0, 0, 135, 327), kFileBrowserUIOverwriteDialogTemplateID, &substitutions);

						if (subHit == kOverwriteNoButton)
							hit = -1;
					}
				}
			}

			if (mode == Mode_Open && hit == kDeleteButton)
			{
				PLDrivers::GetSystemServices()->Beep();
				int16_t subHit = FileBrowserUIImpl::PopUpAlert(Rect::Create(0, 0, 135, 327), kFileBrowserUIDeleteDialogTemplateID, &substitutions);

				if (subHit == kOverwriteYesButton)
				{
					PLPasStr uiFileName = uiImpl.GetSelectedFileName();

					if (composites)
						PortabilityLayer::FileManager::GetInstance()->DeleteCompositeFile(dirID, uiFileName);
					else
						PortabilityLayer::FileManager::GetInstance()->DeleteNonCompositeFile(dirID, uiFileName, extension);

					uiImpl.RemoveSelectedFile();
					dialog->GetItems()[kOkayButton - 1].GetWidget()->SetEnabled(false);
					dialog->GetItems()[kDeleteButton - 1].GetWidget()->SetEnabled(false);
				}
				hit = -1;
			}
		} while (hit != kOkayButton && hit != kCancelButton);

		WindowManager::GetInstance()->SwapExclusiveWindow(exclWindow);

		bool confirmed = false;
		PLPasStr uiFileName;

		if (hit == kOkayButton)
		{
			if (mode == Mode_Open)
			{
				uiFileName = uiImpl.GetSelectedFileName();
				confirmed = true;
			}
			else if (mode == Mode_Save)
			{
				uiFileName = editbox->GetString();
				confirmed = true;
			}
		}

		if (confirmed)
		{
			if (uiFileName.Length() > pathCapacity)
				confirmed = false;
		}

		if (confirmed)
		{
			memcpy(path, uiFileName.Chars(), uiFileName.Length());
			outPathLength = uiFileName.Length();
		}

		dialog->Destroy();

		return confirmed;
	}
}

PL_IMPLEMENT_FILTER_FUNCTION(FileBrowserUIImpl_FileBrowserUIFilter)
PL_IMPLEMENT_FILTER_FUNCTION(FileBrowserUIImpl_PopUpAlertUIFilter)
