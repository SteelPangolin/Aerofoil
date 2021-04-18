//============================================================================
//----------------------------------------------------------------------------
//								Glider PRO 1.0.4
//								by  john calhoun
//----------------------------------------------------------------------------
//============================================================================


#include "WindowDef.h"
#include "BitmapImage.h"
#include "FileManager.h"
#include "Externs.h"
#include "Environ.h"
#include "FontFamily.h"
#include "FontManager.h"
#include "GpApplicationName.h"
#include "GpRenderedFontMetrics.h"
#include "IGpLogDriver.h"
#include "IGpMutex.h"
#include "IGpThreadEvent.h"
#include "IGpDisplayDriver.h"
#include "IGpSystemServices.h"
#include "GpIOStream.h"
#include "House.h"
#include "MainMenuUI.h"
#include "MemoryManager.h"
#include "MenuManager.h"
#include "QDPixMap.h"
#include "QDStandardPalette.h"
#include "RenderedFont.h"
#include "ResolveCachingColor.h"
#include "ResourceManager.h"
#include "PLTimeTaggedVOSEvent.h"
#include "Utilities.h"
#include "Vec2i.h"
#include "WindowManager.h"
#include "WorkerThread.h"

#include "PLApplication.h"
#include "PLDrivers.h"
#include "PLKeyEncoding.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"

#include <atomic>

#define kPrefsVersion			0x0039


void ReadInPrefs (void);
void WriteOutPrefs (void);
void HandleSplashResolutionChange (void);
int main(int argc, const char **argv);


short		isVolume, wasVolume;
short		isDepthPref, dataResFile, numSMWarnings;
Boolean		quitting, doZooms, quickerTransitions, isUseICCProfile, isPrefsLoaded;
Boolean		isAutoScale = true;


extern Str31		highBanner;
extern Str15		leftName, rightName, batteryName, bandName;
extern Str15		highName;
//extern long		encryptedNumber;
extern short		maxFiles, numNeighbors, willMaxFiles;
extern PortabilityLayer::CompositeFile *houseCFile;
extern short		isEditH, isEditV, isMapH, isMapV;
extern short		isToolsH, isToolsV, isCoordH, isCoordV;
extern short		isLinkH, isLinkV, toolMode, mapLeftRoom, mapTopRoom;
extern short		mapRoomsWide, mapRoomsHigh, wasFloor, wasSuite;
extern Boolean		isMusicOn, isSoundOn, isPlayMusicIdle, isHouseChecks;
extern Boolean		houseOpen, isDoColorFade, isEscPauseKey;
extern Boolean		autoRoomEdit, doAutoDemo, doBackground;
extern Boolean		isMapOpen, isToolsOpen, isCoordOpen;
extern Boolean		doPrettyMap, doComplainDialogs;
//extern Boolean		didValidation;

THandle<void>		globalModulePrefs;

//==============================================================  Functions
//--------------------------------------------------------------  ReadInPrefs

// Called only once when game launches - reads in the preferences saved?
// from the last time Glider PRO was launched.  If no prefs are found,?
// it assigns default settings.

void ReadInPrefs (void)
{
	prefsInfo	thePrefs;

	THandle<void> modulePrefs;

	if (LoadPrefs(&thePrefs, &modulePrefs, kPrefsVersion))
	{
#ifdef COMPILEDEMO
		PasStringCopy("\pDemo House", thisHouseName);
#else
		PasStringCopy(thePrefs.wasDefaultName, thisHouseName);
#endif
		PasStringCopy(thePrefs.wasLeftName, leftName);
		PasStringCopy(thePrefs.wasRightName, rightName);
		PasStringCopy(thePrefs.wasBattName, batteryName);
		PasStringCopy(thePrefs.wasBandName, bandName);
		PasStringCopy(thePrefs.wasHighName, highName);
		PasStringCopy(thePrefs.wasHighBanner, highBanner);
		theGlider.leftKey = thePrefs.wasLeftMap;
		theGlider.rightKey = thePrefs.wasRightMap;
		theGlider.battKey = thePrefs.wasBattMap;
		theGlider.bandKey = thePrefs.wasBandMap;
		theGlider.gamepadLeftKey = thePrefs.wasGPLeftMap;
		theGlider.gamepadRightKey = thePrefs.wasGPRightMap;
		theGlider.gamepadBandKey = thePrefs.wasGPBandMap;
		theGlider.gamepadBattKey = thePrefs.wasGPBattMap;
		theGlider.gamepadFlipKey = thePrefs.wasGPFlipMap;
		theGlider.gamepadFaceRightKey = thePrefs.wasGPFaceRightMap;
		theGlider.gamepadFaceLeftKey = thePrefs.wasGPFaceLeftMap;
#ifndef COMPILEDEMO
#ifndef COMPILENOCP
		encryptedNumber = thePrefs.encrypted;
#endif			// COMPILENOCP
#endif			// COMPILEDEMO
		isVolume = thePrefs.wasVolume;
		isDepthPref = thePrefs.wasDepthPref;
		isMusicOn = thePrefs.wasMusicOn;
		doZooms = thePrefs.wasZooms;
		quickerTransitions = thePrefs.wasQuickTrans;
		isDoColorFade = thePrefs.wasDoColorFade;
		isPlayMusicIdle = thePrefs.wasIdleMusic;
		isPlayMusicGame = thePrefs.wasGameMusic;
		isHouseChecks = thePrefs.wasHouseChecks;
		maxFiles = thePrefs.wasMaxFiles;
		if ((maxFiles < 12) || (maxFiles > 500))
			maxFiles = 12;
		isEditH = thePrefs.wasEditH;
		isEditV = thePrefs.wasEditV;
		isMapH = thePrefs.wasMapH;
		isMapV = thePrefs.wasMapV;
		mapRoomsWide = thePrefs.wasMapWide;
		mapRoomsHigh = thePrefs.wasMapHigh;
		isToolsH = thePrefs.wasToolsH;
		isToolsV = thePrefs.wasToolsV;
		isLinkH = thePrefs.wasLinkH;
		isLinkV = thePrefs.wasLinkV;
		isCoordH = thePrefs.wasCoordH;
		isCoordV = thePrefs.wasCoordV;
		mapLeftRoom = thePrefs.isMapLeft;
		mapTopRoom = thePrefs.isMapTop;
		wasFloor = thePrefs.wasFloor;
		wasSuite = thePrefs.wasSuite;
		numSMWarnings = thePrefs.smWarnings;
		autoRoomEdit = thePrefs.wasAutoEdit;
		isMapOpen = thePrefs.wasMapOpen;
		isToolsOpen = thePrefs.wasToolsOpen;
		isCoordOpen = thePrefs.wasCoordOpen;
		numNeighbors = thePrefs.wasNumNeighbors;
		toolMode = thePrefs.wasToolGroup;
		doAutoDemo = thePrefs.wasDoAutoDemo;
		isEscPauseKey = thePrefs.wasEscPauseKey;
		isAutoScale = thePrefs.wasAutoScale;
		isUseICCProfile = thePrefs.wasUseICCProfile;
		doBackground = thePrefs.wasDoBackground;
		doPrettyMap = thePrefs.wasPrettyMap;
		doComplainDialogs = thePrefs.wasComplainDialogs;

		if (modulePrefs)
			ApplyModulePrefs(&modulePrefs);

		globalModulePrefs.Dispose();
		globalModulePrefs = modulePrefs;
		modulePrefs = nullptr;

		isPrefsLoaded = true;
	}
	else
	{
#ifdef COMPILEDEMO
		PasStringCopy("\pDemo House", thisHouseName);
#else
		PasStringCopy(PSTR("Slumberland"), thisHouseName);
#endif
		PasStringCopy(PSTR("lf arrow"), leftName);
		PasStringCopy(PSTR("rt arrow"), rightName);
		PasStringCopy(PSTR("dn arrow"), batteryName);
		PasStringCopy(PSTR("up arrow"), bandName);
		PasStringCopy(PSTR("Your Name"), highName);
		PasStringCopy(PSTR("Your Message Here"), highBanner);
		theGlider.leftKey = PL_KEY_SPECIAL(kLeftArrow);
		theGlider.rightKey = PL_KEY_SPECIAL(kRightArrow);
		theGlider.battKey = PL_KEY_SPECIAL(kDownArrow);
		theGlider.bandKey = PL_KEY_SPECIAL(kUpArrow);
		theGlider.gamepadLeftKey = PL_KEY_GAMEPAD_BUTTON(kDPadLeft, 0);
		theGlider.gamepadRightKey = PL_KEY_GAMEPAD_BUTTON(kDPadRight, 0);
		theGlider.gamepadBandKey = PL_KEY_GAMEPAD_BUTTON(kFaceDown, 0);
		theGlider.gamepadBattKey = PL_KEY_GAMEPAD_BUTTON(kFaceLeft, 0);
		theGlider.gamepadFlipKey = PL_KEY_GAMEPAD_BUTTON(kFaceUp, 0);
		theGlider.gamepadFaceRightKey = PL_KEY_GAMEPAD_BUTTON(kRightBumper, 0);
		theGlider.gamepadFaceLeftKey = PL_KEY_GAMEPAD_BUTTON(kLeftBumper, 0);

		UnivGetSoundVolume(&isVolume, thisMac.hasSM3);
		if (isVolume < 1)
			isVolume = 1;
		else if (isVolume > 3)
			isVolume = 3;

		isDepthPref = kSwitchIfNeeded;
		isSoundOn = true;
		isMusicOn = true;
		isPlayMusicIdle = true;
		isPlayMusicGame = true;
		isHouseChecks = true;
		doZooms = true;
		quickerTransitions = false;
		numNeighbors = 9;
		isDoColorFade = true;
		maxFiles = 48;
		willMaxFiles = 48;
		isEditH = 3;
		isEditV = 41;
		isMapH = 3;
//		isMapV = qd.screenBits.bounds.bottom - 100;
		isMapV = 385;
		mapRoomsWide = 15;
		mapRoomsHigh = 4;
//		isToolsH = qd.screenBits.bounds.right - 120;
		isToolsH = 525;
		isToolsV = 41;
		isLinkH = 50;
		isLinkV = 80;
//		isCoordH = qd.screenBits.bounds.right - 55;
		isCoordH = 50;
		isCoordV = 204;
		mapLeftRoom = 60;
		mapTopRoom = 50;
		wasFloor = 0;
		wasSuite = 0;
		numSMWarnings = 0;
		autoRoomEdit = true;
		isMapOpen = true;
		isToolsOpen = true;
		isCoordOpen = false;
		toolMode = kBlowerMode;
		doAutoDemo = true;
		isEscPauseKey = false;
		isAutoScale = true;
		isUseICCProfile = true;
		doBackground = false;
		doPrettyMap = false;
		doComplainDialogs = true;

		IGpDisplayDriver *displayDriver = PLDrivers::GetDisplayDriver();
		if (PLDrivers::GetSystemServices()->IsFullscreenPreferred() != displayDriver->IsFullScreen())
			displayDriver->RequestToggleFullScreen(0);

		modulePrefs.Dispose();

		isPrefsLoaded = false;
	}

	if ((numNeighbors > 1) && (thisMac.constrainedScreen.right <= 512))
		numNeighbors = 1;

	UnivGetSoundVolume(&wasVolume, thisMac.hasSM3);
	UnivSetSoundVolume(isVolume, thisMac.hasSM3);

	if (isVolume == 0)
		isSoundOn = false;
	else
		isSoundOn = true;
}

//--------------------------------------------------------------  WriteOutPrefs

// Called just before Glider PRO quits.  This function writes out?
// the user preferences to disk.

void WriteOutPrefs (void)
{
	prefsInfo	thePrefs;

	UnivGetSoundVolume(&isVolume, thisMac.hasSM3);

#ifdef COMPILEDEMO
	PasStringCopy("\pDemo House", thePrefs.wasDefaultName);
#else
	PasStringCopy(thisHouseName, thePrefs.wasDefaultName);
#endif
	PasStringCopy(leftName, thePrefs.wasLeftName);
	PasStringCopy(rightName, thePrefs.wasRightName);
	PasStringCopy(batteryName, thePrefs.wasBattName);
	PasStringCopy(bandName, thePrefs.wasBandName);
	PasStringCopy(highName, thePrefs.wasHighName);
	PasStringCopy(highBanner, thePrefs.wasHighBanner);
	thePrefs.wasLeftMap = theGlider.leftKey;
	thePrefs.wasRightMap = theGlider.rightKey;
	thePrefs.wasBattMap = theGlider.battKey;
	thePrefs.wasBandMap = theGlider.bandKey;
	thePrefs.wasGPLeftMap = theGlider.gamepadLeftKey;
	thePrefs.wasGPRightMap = theGlider.gamepadRightKey;
	thePrefs.wasGPBattMap = theGlider.gamepadBattKey;
	thePrefs.wasGPBandMap = theGlider.gamepadBandKey;
	thePrefs.wasGPFlipMap = theGlider.gamepadFlipKey;
	thePrefs.wasGPFaceLeftMap = theGlider.gamepadFaceLeftKey;
	thePrefs.wasGPFaceRightMap = theGlider.gamepadFaceRightKey;
#ifndef COMPILEDEMO
#ifndef COMPILENOCP
	thePrefs.encrypted = encryptedNumber;
	thePrefs.fakeLong = Random();
#endif			// COMPILENOCP
#endif			// COMPILEDEMO
	thePrefs.wasVolume = isVolume;
	thePrefs.wasDepthPref = isDepthPref;
	thePrefs.wasMusicOn = isMusicOn;
	thePrefs.wasZooms = doZooms;
	thePrefs.wasQuickTrans = quickerTransitions;
	thePrefs.wasDoColorFade = isDoColorFade;
	thePrefs.wasIdleMusic = isPlayMusicIdle;
	thePrefs.wasGameMusic = isPlayMusicGame;
	thePrefs.wasHouseChecks = isHouseChecks;
	thePrefs.wasMaxFiles = willMaxFiles;
	thePrefs.wasEditH = isEditH;
	thePrefs.wasEditV = isEditV;
	thePrefs.wasMapH = isMapH;
	thePrefs.wasMapV = isMapV;
	thePrefs.wasMapWide = mapRoomsWide;
	thePrefs.wasMapHigh = mapRoomsHigh;
	thePrefs.wasToolsH = isToolsH;
	thePrefs.wasToolsV = isToolsV;
	thePrefs.isMapLeft = mapLeftRoom;
	thePrefs.isMapTop = mapTopRoom;
	thePrefs.wasFloor = wasFloor;
	thePrefs.wasSuite = wasSuite;
	thePrefs.wasLinkH = isLinkH;
	thePrefs.wasLinkV = isLinkV;
	thePrefs.wasCoordH = isCoordH;
	thePrefs.wasCoordV = isCoordV;
	thePrefs.smWarnings = numSMWarnings;
	thePrefs.wasAutoEdit = autoRoomEdit;
	thePrefs.wasMapOpen = isMapOpen;
	thePrefs.wasToolsOpen = isToolsOpen;
	thePrefs.wasCoordOpen = isCoordOpen;
	thePrefs.wasNumNeighbors = numNeighbors;
	thePrefs.wasToolGroup = toolMode;
	thePrefs.wasDoAutoDemo = doAutoDemo;
	thePrefs.wasEscPauseKey = isEscPauseKey;
	thePrefs.wasAutoScale = isAutoScale;
	thePrefs.wasUseICCProfile = isUseICCProfile;
	thePrefs.wasDoBackground = doBackground;
	thePrefs.wasPrettyMap = doPrettyMap;
	thePrefs.wasComplainDialogs = doComplainDialogs;

	THandle<void> modulePrefs;

	if (!SaveModulePrefs(globalModulePrefs, &modulePrefs) || !SavePrefs(&thePrefs, &modulePrefs, kPrefsVersion))
		SysBeep(1);

	modulePrefs.Dispose();
}

void ShowInitialLaunchDisclaimer()
{
	const char *disclaimerLines[] =
	{
		GP_APPLICATION_NAME " is a port of John Calhoun\xd5s Glider PRO, based",
		"on the 2016 release of the game\xd5s source code and assets.",
		"",
		"Glider PRO, a sequel to the original Glider, was released in 1994",
		"for Apple Macintosh computers, and is widely recognized as one of",
		"of the most iconic Macintosh-exclusive games of the 1990\xd5s.",
		"",
		"I hope that by adapting it to be playable on modern systems, more",
		"people can appreciate this important piece of video game history.",
		"",
		"This software is an adaptation that attempts to restore the original work",
		"as accurately as possible, but some fonts, graphics, and sounds have been",
		"substituted or removed for copyright reasons, and some user interface",
		"components have been added or changed to improve compatibility.",
		"",
		"This software is not developed by, maintained by, supported by, endorsed by,",
		"or otherwise associated with the authors or publishers of Glider PRO.",
		"Any references to Glider PRO in this software are for historical accuracy",
		"and should not be interpreted as implying any form of endorsement."
	};

	const size_t numLines = sizeof(disclaimerLines) / sizeof(disclaimerLines[0]);

	PortabilityLayer::RenderedFont *rfont = GetFont(PortabilityLayer::FontPresets::kApplication14);
	PortabilityLayer::RenderedFont *buttonFont = GetFont(PortabilityLayer::FontPresets::kApplication18);

	const int kButtonSpacing = 16;
	const int kButtonHeight = 32;

	const PLPasStr buttonText = PLDrivers::GetSystemServices()->IsTouchscreen() ? PLPasStr(PSTR("Tap to Continue...")) : PLPasStr(PSTR("Click to Continue..."));
	const int32_t buttonTextWidth = buttonFont->MeasureString(buttonText.UChars(), buttonText.Length());
	const int32_t buttonWidth = buttonTextWidth + 16;

	const int32_t linegap = rfont->GetMetrics().m_linegap;
	const int32_t ascent = rfont->GetMetrics().m_ascent;
	int32_t windowHeight = linegap * numLines + kButtonSpacing + kButtonHeight;

	size_t widestLine = 0;
	for (size_t i = 0; i < numLines; i++)
	{
		const size_t lineWidth = rfont->MeasureCharStr(disclaimerLines[i], strlen(disclaimerLines[i]));
		if (lineWidth > widestLine)
			widestLine = lineWidth;
	}

	if (widestLine > 640)
		widestLine = 640;

	Rect windowRect = Rect::Create(0, 0, windowHeight, widestLine);

	PortabilityLayer::WindowDef def = PortabilityLayer::WindowDef::Create(windowRect, PortabilityLayer::WindowStyleFlags::kBorderless, true, 0, 0, PSTR(""));

	Window *disclaimerWindow = PortabilityLayer::WindowManager::GetInstance()->CreateWindow(def);
	PortabilityLayer::WindowManager::GetInstance()->PutWindowBehind(disclaimerWindow, PL_GetPutInFrontWindowPtr());

	DrawSurface *surface = disclaimerWindow->GetDrawSurface();
	PortabilityLayer::ResolveCachingColor blackColor(StdColors::Black());
	PortabilityLayer::ResolveCachingColor whiteColor(StdColors::White());

	surface->FillRect(windowRect, blackColor);

	int32_t wx = (static_cast<int32_t>(thisMac.fullScreen.Width()) - static_cast<int32_t>(surface->m_port.GetRect().Width())) / 2;
	int32_t wy = (static_cast<int32_t>(thisMac.fullScreen.Height()) - static_cast<int32_t>(surface->m_port.GetRect().Height())) / 2;

	disclaimerWindow->SetPosition(PortabilityLayer::Vec2i(wx, wy));


	for (size_t i = 0; i < numLines; i++)
	{
		const size_t lineWidth = rfont->MeasureCharStr(disclaimerLines[i], strlen(disclaimerLines[i]));
		if (lineWidth > widestLine)
			widestLine = lineWidth;

		int32_t xOffset = static_cast<int32_t>((widestLine - lineWidth) / 2);

		Point drawPt = Point::Create(xOffset, linegap * i + (linegap + ascent) / 2);
		surface->DrawString(drawPt, PLPasStr(strlen(disclaimerLines[i]), disclaimerLines[i]), whiteColor, rfont);
	}

	const int numTicksToEnable = 10 * 60;
	const int borderThickness = 2;

	Rect buttonRect = Rect::Create(windowRect.bottom - kButtonHeight, windowRect.right - buttonWidth, windowRect.bottom, windowRect.right);

	{
		int topYOffset = (buttonRect.Height() - borderThickness) / 2;
		surface->FillRect(Rect::Create(buttonRect.top + topYOffset, buttonRect.left, buttonRect.top + topYOffset + borderThickness, buttonRect.left + borderThickness), whiteColor);
		surface->FillRect(Rect::Create(buttonRect.top + topYOffset, buttonRect.right - borderThickness, buttonRect.top + topYOffset + borderThickness, buttonRect.right), whiteColor);
	}


	for (int i = 0; i < numTicksToEnable; i++)
	{
		int bar = (buttonWidth - borderThickness * 6) * i / numTicksToEnable;

		int topYOffset = (buttonRect.Height() - borderThickness) / 2;
		surface->FillRect(Rect::Create(buttonRect.top + topYOffset, buttonRect.left + borderThickness * 3, buttonRect.top + topYOffset + borderThickness, buttonRect.left + borderThickness * 3 + bar), whiteColor);

		Delay(1, nullptr);
	}

	const int numTicksToExpand = 15;
	for (int i = 0; i < numTicksToExpand; i++)
	{
		int height = buttonRect.Height() * i / numTicksToExpand;
		int topYOffset = (buttonRect.Height() - height) / 2;
		surface->FillRect(Rect::Create(buttonRect.top + topYOffset, buttonRect.left, buttonRect.top + topYOffset + height, buttonRect.right), whiteColor);

		Delay(1, nullptr);
	}

	Point textPoint;
	textPoint.h = (buttonRect.left + buttonRect.right - buttonTextWidth) / 2;
	textPoint.v = (buttonRect.top + buttonRect.bottom + ascent) / 2;

	const int numTicksToFade = 15;
	for (int i = 0; i <= numTicksToFade; i++)
	{
		int intensity = 255 - (i * 255 / numTicksToFade);
		PortabilityLayer::ResolveCachingColor backgroundColor(PortabilityLayer::RGBAColor::Create(intensity, intensity, intensity, 255));

		surface->FillRect(buttonRect, whiteColor);
		surface->FillRect(buttonRect.Inset(borderThickness, borderThickness), backgroundColor);

		surface->DrawString(textPoint, buttonText, whiteColor, buttonFont);

		Delay(1, nullptr);
	}

	bool dismiss = false;

	FlushEvents();

	while (!dismiss)
	{
		TimeTaggedVOSEvent evt;
		while (WaitForEvent(&evt, 1))
		{
			if (evt.IsLMouseDownEvent())
				dismiss = true;
		}
	}

	Delay(1, nullptr);

	PortabilityLayer::WindowManager::GetInstance()->DestroyWindow(disclaimerWindow);

	Delay(60, nullptr);
}

void gpAppInit()
{
	// This is called before the display driver is initialized
	InstallResolutionHandler();
}

//--------------------------------------------------------------  main
// Here is main().  The first function called when Glider PRO comes up.

int AppStartup()
{
	//	long		wasSeed;
	long		theErr;
	PLError_t		fileErr;
	Boolean		whoCares, copyGood;

	PL_Init();

	IGpLogDriver *logger = PLDrivers::GetLogDriver();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 1...");

	ToolBoxInit();
	CheckOurEnvirons();

	if (thisMac.isTouchscreen)
		PortabilityLayer::MenuManager::GetInstance()->SetMenuTouchScreenStyle(true);

	if (!thisMac.hasColor)
		RedAlert(kErrNeedColorQD);
	if (!thisMac.hasSystem7)
		RedAlert(kErrNeedSystem7);
	if (thisMac.numScreens == 0)
		RedAlert(kErrNeed16Or256Colors);
	//	dataResFile = OpenResFile("\pMermaid");
	SetUpAppleEvents();
	LoadCursors();
	ReadInPrefs();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 2...");

	SpinCursor(2);	// Tick once to let the display driver flush any resolution changes from prefs
	FlushResolutionChange();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 3...");

#if defined COMPILEDEMO
	copyGood = true;
#elif defined COMPILENOCP
	//	didValidation = false;
	copyGood = true;
#else
	didValidation = false;
	copyGood = ValidInstallation(true);
	if (!copyGood)
		encryptedNumber = 0L;
	else if (didValidation)
		WriteOutPrefs();				SpinCursor(3);
#endif

	//	if ((thisMac.numScreens > 1) && (isUseSecondScreen))
	//		ReflectSecondMonitorEnvirons(false, true, true);
	HandleDepthSwitching();
	VariableInit();
	GetExtraCursors();
	InitMarquee();
	CreatePointers();
	InitSrcRects();
	CreateOffscreens();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 3...");

	bool resolutionChanged = false;

	if (!isPrefsLoaded)
	{
		WriteOutPrefs();

		if (thisMac.isResolutionDirty)
		{
			resolutionChanged = true;
			FlushResolutionChange();
		}

		//ShowInitialLaunchDisclaimer();
	}

	if (thisMac.isResolutionDirty)
		resolutionChanged = true;

	if (resolutionChanged)
		HandleSplashResolutionChange();
	else
		OpenMainWindow();

	if (isDoColorFade)
		PortabilityLayer::WindowManager::GetInstance()->SetWindowDesaturation(mainWindow, 1.0);

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 4...");

	InitSound();						SpinCursor(2);
	InitMusic();						SpinCursor(2);

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Init phase 5...");

	BuildHouseList();
	OpenHouse(true);

	PlayPrioritySound(kBirdSound, kBirdPriority);
	{
		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		DelayTicks(6);
	}
	InitializeMenus();					InitCursor();

#if BUILD_ARCADE_VERSION
	//	HideMenuBarOld();
#endif

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "Initialization completed");

	if (isDoColorFade)
		WashColorIn();

	UpdateMainWindow();

	if (thisMac.isTouchscreen)
		StartMainMenuUI();

	return 0;
}

int AppShutdown()
{
	/*
	#if BUILD_ARCADE_VERSION
		ShowMenuBarOld();
	#endif
	*/
	KillMusic();
	KillSound();
	if (houseOpen)
	{
		if (!CloseHouse())
		{
			CloseHouseResFork();
			if (houseCFile)
				houseCFile->Close();
			houseOpen = false;
		}
	}
	WriteOutPrefs();
	PL_DEAD(FlushEvents());
	//	theErr = LoadScrap();

	return 0;
}

int gpAppMain()
{
	int returnCode = AppStartup();
	if (returnCode != 0)
		return returnCode;

	while (!quitting)		// this is the main loop
		HandleEvent();

	return AppShutdown();
}
