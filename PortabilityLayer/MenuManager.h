#pragma once

#include <stdint.h>
#include <stddef.h>

#include "PLErrorCodes.h"
#include "CoreDefs.h"

template<class T>
class THandle;

class PLPasStr;
struct IGpDisplayDriver;
struct Menu;

namespace PortabilityLayer
{
	struct Vec2i;

	class MenuManager
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual THandle<Menu> DeserializeMenu(const void *resData) const = 0;
		virtual THandle<Menu> GetMenuByID(int id) const = 0;

		virtual PLError_t AppendMenuItem(const THandle<Menu> &menu, int8_t iconResID, uint8_t key, uint8_t submenuID, uint8_t textStyle, bool enabled, bool checked, const PLPasStr &text) const = 0;

		virtual void InsertMenuBefore(const THandle<Menu> &insertingMenu, const THandle<Menu> &existingMenu) = 0;
		virtual void InsertMenuAfter(const THandle<Menu> &insertingMenu, const THandle<Menu> &existingMenu) = 0;
		virtual void InsertMenuAtEnd(const THandle<Menu> &insertingMenu) = 0;
		virtual void InsertMenuAtBeginning(const THandle<Menu> &insertingMenu) = 0;

		virtual void RemoveMenu(const THandle<Menu> &menu) = 0;

		virtual void SetMenuEnabled(const THandle<Menu> &menuHandle, bool enabled) = 0;
		virtual void SetItemEnabled(const THandle<Menu> &menu, unsigned int index, bool enabled) = 0;
		virtual void SetItemChecked(const THandle<Menu> &menu, unsigned int index, bool checked) = 0;
		virtual bool SetItemText(const THandle<Menu> &menu, unsigned int index, const PLPasStr &str) = 0;

		virtual bool GetMenuEnabled(const THandle<Menu> &menuHandle) const = 0;
		virtual bool GetItemEnabled(const THandle<Menu> &menu, unsigned int index) const = 0;
		virtual bool GetItemChecked(const THandle<Menu> &menu, unsigned int index) const = 0;
		virtual PLPasStr GetItemText(const THandle<Menu> &menu, unsigned int index) const = 0;

		virtual bool IsPointInMenuBar(const Vec2i &point) const = 0;
		virtual uint16_t GetMenuBarHeight() const = 0;

		virtual bool FindMenuShortcut(uint16_t &menuID, uint16_t &itemID, uint8_t shortcutChar) = 0;
		GP_ASYNCIFY_PARANOID_VIRTUAL void MenuSelect(const Vec2i &initialPoint, int16_t *outMenu, uint16_t *outItem) GP_ASYNCIFY_PARANOID_PURE;
		virtual void PopupMenuSelect(const THandle<Menu> &menu, const Vec2i &popupMenuPos, const Vec2i &initialPoint, size_t initialItem, uint16_t *outItem) = 0;

		virtual void DrawMenuBar() = 0;
		virtual void SetMenuVisible(bool isVisible) = 0;

		virtual void RenderFrame(IGpDisplayDriver *displayDriver) = 0;

		virtual void SetMenuTouchScreenStyle(bool isTouchScreenStyle) = 0;
		virtual bool IsMenuTouchScreenStyle() const = 0;

		static MenuManager *GetInstance();
	};
}
