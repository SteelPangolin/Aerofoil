#include "PLMenus.h"
#include "PLResources.h"

#include "MenuManager.h"

// Menu resource structure:
// uint16 menu ID
// uint16 width
// uint16 height
// uint16 command ID
// uint16 padding
// uint32 enable flags
// pstr title (Apple menu is 0x14)
// Zero-byte terminated list:
//     pstr Item text (separator is char 0x2d)
//     int8 Icon #
//     uint8 key
//     uint8 submenu ID
//     uint8 text style

MenuHandle GetMenu(int resID)
{
	Handle menuRes = GetResource('MENU', resID);

	if (!menuRes)
		return nullptr;

	const MenuHandle menu = PortabilityLayer::MenuManager::GetInstance()->DeserializeMenu(*menuRes);
	ReleaseResource(menuRes);

	return menu;
}

void InsertMenu(MenuHandle menu, int beforeID)
{
	PortabilityLayer::MenuManager *mm = PortabilityLayer::MenuManager::GetInstance();
	if (beforeID)
	{
		MenuHandle otherMenu = mm->GetMenuByID(beforeID);
		if (otherMenu)
		{
			mm->InsertMenuBefore(menu, otherMenu);
			return;
		}
	}

	mm->InsertMenuAtEnd(menu);
}

void DeleteMenu(int menuID)
{
	PL_NotYetImplemented();
}

void DrawMenuBar()
{
	PL_NotYetImplemented_TODO("Menus");
}

void HiliteMenu(int menu)
{
	PL_NotYetImplemented_TODO("Menus");
}

void EnableMenuItem(MenuHandle menu, int index)
{
	PL_NotYetImplemented_TODO("Menus");
}

void DisableMenuItem(MenuHandle menu, int index)
{
	PL_NotYetImplemented_TODO("Menus");
}

void CheckMenuItem(MenuHandle menu, int index, Boolean checked)
{
	PL_NotYetImplemented_TODO("Menus");
}

void SetMenuItemText(MenuHandle menu, int index, const PLPasStr &text)
{
	PL_NotYetImplemented_TODO("Menus");
}