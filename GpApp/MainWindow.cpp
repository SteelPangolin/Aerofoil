//============================================================================
//----------------------------------------------------------------------------
//								MainWindow.c
//----------------------------------------------------------------------------
//============================================================================


#include "Externs.h"
#include "Environ.h"
#include "FontFamily.h"
#include "House.h"
#include "IGpDisplayDriver.h"
#include "InputManager.h"
#include "MenuManager.h"
#include "QDPixMap.h"
#include "RectUtils.h"
#include "PLKeyEncoding.h"
#include "PLStandardColors.h"
#include "ResolveCachingColor.h"
#include "WindowDef.h"
#include "WindowManager.h"

#include "PLDrivers.h"
#include "PLNumberFormatting.h"
#include "PLPasStr.h"
#include "PLSysCalls.h"

#define kMainWindowID			128
#define kEditWindowID			129
#define kMenuWindowID			130


void DrawOnSplash (DrawSurface *surface);
void KeepWindowInBounds(Window *window);

CTabHandle		theCTab;
PixMapHandle	thePMap;
ColorSpec *		wasColors;
ColorSpec *		newColors;
IGpCursor		*handCursor, *vertCursor, *horiCursor;
IGpCursor		*diagCursor;
Rect			workSrcRect;
DrawSurface		*workSrcMap;
Rect			mainWindowRect;
WindowPtr		mainWindow, boardWindow;
short			isEditH, isEditV;
short			playOriginH, playOriginV;
short			splashOriginH, splashOriginV;
short			theMode;
Boolean			fadeGraysOut, isDoColorFade, splashDrawn;

extern	short		toolSelected;
extern	Boolean		noRoomAtAll, isUseSecondScreen;
extern	Boolean		quickerTransitions, houseIsReadOnly;
extern	Boolean		menusUp;
extern	Boolean		houseOpen;

//==============================================================  Functions
//--------------------------------------------------------------  DrawOnSplash

// Draws additional text on top of splash screen.

void DrawOnSplash(DrawSurface *surface)
{
	Str255		houseLoadedStr;

	if (!houseOpen)
		return;
	
	PasStringCopy(PSTR("House: "), houseLoadedStr);
	PasStringConcat(houseLoadedStr, thisHouseName);
	if ((thisMac.hasQT) && (hasMovie))
		PasStringConcat(houseLoadedStr, PSTR(" (TV)"));

	PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication9Bold);

	const Point textPoint = Point::Create(splashOriginH + 436, splashOriginV + 314);
	if (thisMac.isDepth == 4)
	{
		PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
		surface->DrawString(textPoint, houseLoadedStr, whiteColor, appFont);
	}
	else
	{
		if (houseIsReadOnly)
			ColorText(surface, textPoint, houseLoadedStr, 5L, appFont);
		else
			ColorText(surface, textPoint, houseLoadedStr, 28L, appFont);
	}
}

//--------------------------------------------------------------  RedrawSplashScreen

void RedrawSplashScreen (void)
{
	Rect		tempRect;
	DrawSurface	*surface = workSrcMap;

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	surface->FillRect(workSrcRect, blackColor);

	QSetRect(&tempRect, 0, 0, 640, 460);
	QOffsetRect(&tempRect, splashOriginH, splashOriginV);
	LoadScaledGraphic(surface, kSplash8BitPICT, &tempRect);
	DrawOnSplash(surface);

	CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap),
		GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()),
		&workSrcRect, &workSrcRect, srcCopy);

//	if (quickerTransitions)
//		DissBitsChunky(&workSrcRect);
//	else
//		DissBits(&workSrcRect);
	CopyRectMainToWork(&workSrcRect);

	mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

	if (!thisMac.isTouchscreen)
	{
		menusUp = true;
		PortabilityLayer::MenuManager::GetInstance()->SetMenuVisible(true);
	}
}

//--------------------------------------------------------------  UpdateMainWindow

// Redraws the main window (depends on mode were in - splash, editing, playing).

void UpdateMainWindow (void)
{
	Rect		tempRect;
	
	if (theMode == kEditMode)
	{
		PauseMarquee();
		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap),
				GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()), 
				&mainWindowRect, &mainWindowRect, srcCopy);
		ResumeMarquee();
	}
	else if ((theMode == kSplashMode) || (theMode == kPlayMode))
	{
		PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
		workSrcMap->FillRect(workSrcRect, blackColor);

		QSetRect(&tempRect, 0, 0, 640, 460);
		QOffsetRect(&tempRect, splashOriginH, splashOriginV);
		LoadScaledGraphic(workSrcMap, kSplash8BitPICT, &tempRect);
		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap), 
				GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()), 
				&workSrcRect, &mainWindowRect, srcCopy);
		
		DrawOnSplash(mainWindow->GetDrawSurface());
	}

	mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
	
	splashDrawn = true;
}

//--------------------------------------------------------------  OpenMainWindow
// Opens up the main window (how it does this depends on mode were in).

void OpenMainWindow (void)
{
//	long		wasSeed;
	short		whichRoom;

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

	if (mainWindow != nil)
	{
		YellowAlert(kYellowUnaccounted, 6);
		return;
	}
	
	if (theMode == kEditMode)
	{
		PortabilityLayer::WindowManager::GetInstance()->SetBackgroundColor(51, 51, 102);

		QSetRect(&mainWindowRect, 0, 0, 512, 322);
		mainWindow = GetNewCWindow(kEditWindowID, nil, kPutInFront);
		SizeWindow(mainWindow, mainWindowRect.right, 
				mainWindowRect.bottom, false);
		
		if (OptionKeyDown())
		{
			isEditH = 10;
			isEditV = 46;
		}
		MoveWindow(mainWindow, isEditH, isEditV, true);
		ShowWindow(mainWindow);

		DrawSurface *mainWindowSurface = mainWindow->GetDrawSurface();
		
		whichRoom = GetFirstRoomNumber();
		CopyRoomToThisRoom(whichRoom);
		ReflectCurrentRoom(false);

		KeepWindowInBounds(mainWindow);
	}
	else
	{
#if !GP_DEBUG_CONFIG
		PortabilityLayer::WindowManager::GetInstance()->SetBackgroundColor(0, 0, 0);
#else
		PortabilityLayer::WindowManager::GetInstance()->SetBackgroundColor(51, 0, 0);
#endif

		if (boardWindow == nil)
		{
			PortabilityLayer::WindowManager *windowManager = PortabilityLayer::WindowManager::GetInstance();

			Rect scorebarRect = thisMac.constrainedScreen;
			scorebarRect.bottom = scorebarRect.top + kScoreboardTall;

			PortabilityLayer::WindowDef windowDef = PortabilityLayer::WindowDef::Create(scorebarRect, PortabilityLayer::WindowStyleFlags::kBorderless, true, 0, 0, PSTR("Scoreboard"));
			boardWindow = windowManager->CreateWindow(windowDef);
			if (boardWindow != nil)
				windowManager->PutWindowBehind(boardWindow, PL_GetPutInFrontWindowPtr());
			else
				PL_NotYetImplemented_TODO("Errors");

			PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
			boardWindow->GetDrawSurface()->FillRect(boardWindow->GetSurfaceRect(), blackColor);
		}
		mainWindowRect = thisMac.constrainedScreen;
		ZeroRectCorner(&mainWindowRect);
		mainWindowRect.bottom -= kScoreboardTall;		// thisMac.menuHigh
		mainWindow = GetNewCWindow(kMainWindowID, nil, kPutInFront);
		SizeWindow(mainWindow, mainWindowRect.right - mainWindowRect.left, 
				mainWindowRect.bottom - mainWindowRect.top, false);

		const short mainWindowLeft = (thisMac.fullScreen.left + thisMac.fullScreen.right + thisMac.constrainedScreen.left - thisMac.constrainedScreen.right) / 2;
		short mainWindowTop = (thisMac.fullScreen.top + thisMac.fullScreen.bottom + thisMac.constrainedScreen.top - thisMac.constrainedScreen.bottom) / 2;

		mainWindowTop += kScoreboardTall;

		MoveWindow(boardWindow, mainWindowLeft, 0, true);
		MoveWindow(mainWindow, mainWindowLeft, mainWindowTop, true);	// thisMac.menuHigh
		ShowWindow(mainWindow);

		DrawSurface *mainWindowSurface = mainWindow->GetDrawSurface();

//		CopyRgn(mainWindow->clipRgn, mainWindow->visRgn);

		mainWindowSurface->FillRect(mainWindowRect, blackColor);
		
		splashOriginH = ((thisMac.constrainedScreen.right - thisMac.constrainedScreen.left) - 640) / 2;
		if (splashOriginH < 0)
			splashOriginH = 0;
		splashOriginV = ((thisMac.constrainedScreen.bottom - thisMac.constrainedScreen.top) - 480) / 2;
		if (splashOriginV < 0)
			splashOriginV = 0;
		
		workSrcMap->FillRect(workSrcRect, blackColor);
		
//		if ((fadeGraysOut) && (isDoColorFade))
//		{
//			wasSeed = ExtractCTSeed((CGrafPtr)mainWindow);
//			SetPortWindowPort(mainWindow);
//			SetPaletteToGrays();
//			HardDrawMainWindow();
//			fadeGraysOut = false;
//			ForceCTSeed((CGrafPtr)mainWindow, wasSeed);
//		}
	}

	CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap),
		GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()),
		&mainWindowRect, &mainWindowRect, srcCopy);

	UpdateMainWindow();

	mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

//--------------------------------------------------------------  CloseMainWindow

// Closes the main window.

void CloseMainWindow (void)
{
	if (mainWindow != nil)
		PortabilityLayer::WindowManager::GetInstance()->DestroyWindow(mainWindow);
	mainWindow = nil;

	if (boardWindow != nil)
		PortabilityLayer::WindowManager::GetInstance()->DestroyWindow(boardWindow);
	boardWindow = nil;
}

//--------------------------------------------------------------  ZoomBetweenWindows
// Zooms from one window size to another.  Just for effect.

/*
#ifndef COMPILEDEMO
void ZoomBetweenWindows (void)
{
	Rect		aRect;
	short		h, v;
	
	if (theMode == kEditMode)
	{
		QSetRect(&aRect, 0, 0, 512, 342);
		QOffsetRect(&aRect, isEditH, isEditV);
		ZoomRectToRect(&(thisMac.screen), &aRect);
	}
	else
	{
		aRect = mainWindow->portRect;
		GetWindowLeftTop(mainWindow, &h, &v);
		QOffsetRect(&aRect, h, v);
		ZoomRectToRect(&aRect, &(thisMac.screen));
	}
}
#endif
*/

//--------------------------------------------------------------  UpdateEditWindowTitle
// Handles changing the title across the top of the main window.  OnlyÉ
// relevant when editing a house (room title displayed in window title).

#ifndef COMPILEDEMO
void UpdateEditWindowTitle (void)
{
	Str255		newTitle, tempStr;
	
	if (mainWindow == nil)
		return;
	
	PasStringCopy(thisHouseName, newTitle);
	PasStringConcat(newTitle, PSTR(" - "));
	if (noRoomAtAll)
		PasStringConcat(newTitle, PSTR("No rooms"));
	else if (houseUnlocked)
	{
		PasStringConcat(newTitle, thisRoom->name);
		PasStringConcat(newTitle, PSTR(" ("));
		NumToString((long)thisRoom->floor, tempStr);
		PasStringConcat(newTitle, tempStr);
		PasStringConcat(newTitle, PSTR(", "));
		NumToString((long)thisRoom->suite, tempStr);
		PasStringConcat(newTitle, tempStr);
		PasStringConcat(newTitle, PSTR(")"));
	}
	else
		PasStringConcat(newTitle, PSTR("House Locked"));

	PortabilityLayer::WindowManager::GetInstance()->SetWindowTitle(mainWindow, newTitle);
}
#endif

//--------------------------------------------------------------  HandleMainClick

// Handle a mouse click in the main window (relevant only when editing).

void HandleMainClick (Point wherePt, Boolean isDoubleClick)
{
	
	if ((theMode != kEditMode) || (mainWindow == nil) || 
			(!houseUnlocked))
		return;
	
	wherePt -= mainWindow->GetTopLeftCoord();

	DrawSurface *mainWindowSurface = mainWindow->GetDrawSurface();
	
	if (toolSelected == kSelectTool)
		DoSelectionClick(mainWindow, mainWindowSurface, wherePt, isDoubleClick);
	else
		DoNewObjectClick(wherePt);

	const KeyDownStates *theseKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

	if (!theseKeys->IsSet(PL_KEY_EITHER_SPECIAL(kShift)))
	{
		EraseSelectedTool();
		SelectTool(kSelectTool);
	}
}

//--------------------------------------------------------------  ShowMenuBarOld
// Displays the menu bar (after having been hidden).
/*
void ShowMenuBarOld (void)
{
	Rect			theRect;
	GrafPtr			wasPort, tempPort;
	RgnHandle		worldRgn, menuBarRgn;
	
	if (LMGetMBarHeight() == 0)
	{
		GetPort(&wasPort);
		tempPort = (GrafPtr)NewPtrClear(sizeof(GrafPort));
		OpenPort(tempPort);
		SetPort((GrafPtr)tempPort);
		
		LMSetMBarHeight(thisMac.menuHigh);
		
		theRect = (**GetGrayRgn()).rgnBBox;
		UnionRect(&theRect, &qd.screenBits.bounds, &theRect);
		worldRgn = NewRgn();
		OpenRgn();
		FrameRoundRect(&theRect, 16, 16);
		CloseRgn(worldRgn);
		
		theRect = qd.screenBits.bounds;
		theRect.bottom = theRect.top + thisMac.menuHigh;
		menuBarRgn = NewRgn();
		RectRgn(menuBarRgn, &theRect);
		
		SectRgn(worldRgn, menuBarRgn, menuBarRgn);	//	/------------------\	
		DisposeRgn(worldRgn);						//	|__________________|	
		
		UnionRgn(tempPort->visRgn, menuBarRgn, tempPort->visRgn);
		DiffRgn(tempPort->visRgn, menuBarRgn, tempPort->visRgn);
		DisposeRgn(menuBarRgn);
		
		ClosePort(tempPort);
		SetPort((GrafPtr)wasPort);
		
		DrawMenuBar();
	}
}
*/
//--------------------------------------------------------------  HideMenuBarOld
// Hides the menu bar - completely erasing it from the screen.
/*
void HideMenuBarOld (void)
{
	Rect			theRect;
	RgnHandle		worldRgn, menuBarRgn;
	GrafPtr			wasPort, tempPort;
	
	if (LMGetMBarHeight() != 0)
	{
		GetPort(&wasPort);
		tempPort = (GrafPtr)NewPtrClear(sizeof(GrafPort));
		OpenPort(tempPort);
		SetPort((GrafPtr)tempPort);
		
		LMSetMBarHeight(0);
		
		theRect = (**GetGrayRgn()).rgnBBox;
		UnionRect(&theRect, &qd.screenBits.bounds, &theRect);
		worldRgn = NewRgn();
		OpenRgn();
		FrameRoundRect(&theRect, 16, 16);
		CloseRgn(worldRgn);
		
		theRect = qd.screenBits.bounds;
		theRect.bottom = theRect.top + thisMac.menuHigh;
		menuBarRgn = NewRgn();
		RectRgn(menuBarRgn, &theRect);
		
		SectRgn(worldRgn, menuBarRgn, menuBarRgn);	//	/------------------\	
		DisposeRgn(worldRgn);						//	|__________________|	
		
		UnionRgn(tempPort->visRgn, menuBarRgn, tempPort->visRgn);
		DisposeRgn(menuBarRgn);
		
		PaintRect(&theRect);
		
		ClosePort(tempPort);
		SetPort((GrafPtr)wasPort);
	}
}
*/
//--------------------------------------------------------------  SetPaletteToGrays

// Sets up a gray palette corresponding in luminance to the standard colorÉ
// palette.  This is to facilitate the gray->color fade when the game comes up.
/*
void SetPaletteToGrays (void)
{
	GDHandle	theDevice;
	long		longGray;
	short		i;
	char		wasState;
	
	wasState = HGetState((Handle)thisGDevice);
	HLock((Handle)thisGDevice);
	thePMap = (*thisGDevice)->gdPMap;
	HSetState((Handle)thisGDevice, wasState);
	
	theCTab = (*thePMap)->pmTable;
	wasColors = nil;
	wasColors = (ColorSpec*)NewPtr(sizeof(ColorSpec) * 256);
	if (wasColors == nil)
		RedAlert(kErrNoMemory);
	
	newColors = nil;
	newColors = (ColorSpec*)NewPtr(sizeof(ColorSpec) * 256);
	if (newColors == nil)
		RedAlert(kErrNoMemory);
	
	for (i = 0; i < 256; i++)
	{
		wasColors[i] = (*theCTab)->ctTable[i];
		newColors[i] = (*theCTab)->ctTable[i];
		
		if (i != 5)
		{
			longGray = ((long)newColors[i].rgb.red * 3L) / 10L + 
				((long)newColors[i].rgb.green * 6L) / 10L + 
				((long)newColors[i].rgb.blue * 1L) / 10L;
			
			newColors[i].rgb.red = (unsigned short)longGray;
			newColors[i].rgb.green = (unsigned short)longGray;
			newColors[i].rgb.blue = (unsigned short)longGray;
		}
	}
	
	theDevice = GetGDevice();
	SetGDevice(thisGDevice);
	SetEntries(0, 255, newColors);
	SetGDevice(theDevice);
}
*/
//--------------------------------------------------------------  HardDrawMainWindow
// Ignores the ToolBox - this function draws direct to screen in order toÉ
// circumvent the Toolbox's attempt to color-match to the current palette.
/*
void HardDrawMainWindow (void)
{
	PixMapHandle	pixMapH;
	Point			offsetPt;
	long			srcRowBytes, destRowBytes;
	long			src;
	long			dest;
	short			i, w;
	SInt8			mode;
	char			wasState;
	
	wasState = HGetState((Handle)thisGDevice);
	HLock((Handle)thisGDevice);
	pixMapH = (**thisGDevice).gdPMap;
	HSetState((Handle)thisGDevice, wasState);
	
	srcRowBytes = (long)((*(workSrcMap->portPixMap))->rowBytes & 0x7FFF);
	destRowBytes = (**pixMapH).rowBytes & 0x7FFF;
	src = (long)((*(workSrcMap->portPixMap))->baseAddr);
	dest = (long)((**pixMapH).baseAddr) + splashOriginH + 
			((splashOriginV + thisMac.menuHigh) * destRowBytes);
	
	offsetPt.h = 0;
	offsetPt.v = 0;
	ShieldCursor(&mainWindowRect, offsetPt);
	mode = true32b;
	SwapMMUMode(&mode);
	for (i = 0; i < 460; i++)
	{
		for (w = 0; w < 160; w++)
		{
			*(long *)dest = *(long *)src;
			dest += 4L;
			src += 4L;
		}
		src -= 640;
		dest -= 640;
		src += srcRowBytes;
		dest += destRowBytes;
	}
	SwapMMUMode(&mode);
	ShowCursor();
}
*/
//--------------------------------------------------------------  WashColorIn
// Slowly walks the palette from its gray luminance state to the full colorÉ
// palette.  In this way, color appears to slowly wash in.
void WashColorIn (void)
{
	PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();

	#define		kGray2ColorSteps	180
	long		longDelta;
	short		i, c;

	for (i = 0; i < kGray2ColorSteps; i++)
	{
		PortabilityLayer::WindowManager::GetInstance()->SetWindowDesaturation(mainWindow, static_cast<float>(kGray2ColorSteps - i) / static_cast<float>(kGray2ColorSteps));

		Delay(1, nullptr);
		if (PortabilityLayer::InputManager::GetInstance()->GetKeys()->m_mouse.Get(GpMouseButtons::kLeft))
			break;
	}

	PortabilityLayer::WindowManager::GetInstance()->SetWindowDesaturation(mainWindow, 0.0f);
}
