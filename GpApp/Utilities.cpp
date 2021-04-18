//============================================================================
//----------------------------------------------------------------------------
//								  Utilities.c
//----------------------------------------------------------------------------
//============================================================================

#include "PLDialogs.h"
#include "PLEventQueue.h"
#include "PLKeyEncoding.h"
#include "PLQDraw.h"
#include "PLPasStr.h"
#include "PLResources.h"
#include "PLSound.h"
#include "PLSysCalls.h"
#include "PLTimeTaggedVOSEvent.h"
#include "QDPixMap.h"
#include "BitmapImage.h"
#include "DialogManager.h"
#include "DisplayDeviceManager.h"
#include "Externs.h"
#include "IGpLogDriver.h"
#include "IconLoader.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "Utilities.h"

#include "PLDrivers.h"

#include <assert.h>


UInt32		theSeed;


extern	Boolean		switchedOut;


//--------------------------------------------------------------  ToolBoxInit

// The standard ToolBox intialization that must happen when any Mac�
// program first launches.

void ToolBoxInit (void)
{
	InitCursor();
	switchedOut = false;
}

//--------------------------------------------------------------  RandomInt
// Returns a random integer (short) within "range".

short RandomInt (short range)
{
	long	rawResult;

	rawResult = Random();
	if (rawResult < 0L)
		rawResult *= -1L;
	rawResult = (rawResult * (long)range) / 32768L;

	return ((short)rawResult);
}

//--------------------------------------------------------------  RandomLong

// Returns a random long interger within "range".

long RandomLong (long range)
{
	long	highWord, lowWord;
	long	rawResultHi, rawResultLo;

	highWord = (range & 0xFFFF0000) >> 16;
	lowWord = range & 0x0000FFFF;

	rawResultHi = Random();
	if (rawResultHi < 0L)
		rawResultHi *= -1L;
	rawResultHi = (rawResultHi * highWord) / 32768L;

	rawResultLo = Random();
	if (rawResultLo < 0L)
		rawResultLo *= -1L;
	rawResultLo = (rawResultLo * lowWord) / 32768L;

	rawResultHi = (rawResultHi << 16) + rawResultLo;

	return (rawResultHi);
}

//--------------------------------------------------------------  RedAlert

// Called when we must quit app.  Brings up a dialog informing user�
// of the problem and the exits to shell.

void RedAlert (short errorNumber)
{
	IGpLogDriver *logger = PLDrivers::GetLogDriver();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Error, "Red alert error %i", static_cast<int>(errorNumber));

	#define			rDeathAlertID	170		// alert res. ID for death error
	#define			rErrTitleID		170		// string ID for death error title
	#define			rErrMssgID		171		// string ID for death error message
	short			dummyInt;
	Str255			errTitle, errMessage, errNumberString;

	InitCursor();

	if (errorNumber > 1)		// <= 0 is unaccounted for
	{
		GetIndString(errTitle, rErrTitleID, errorNumber);
		GetIndString(errMessage, rErrMssgID, errorNumber);
	}
	else
	{
		GetIndString(errTitle, rErrTitleID, 1);
		GetIndString(errMessage, rErrMssgID, 1);
	}
	NumToString((long)errorNumber, errNumberString);
	DialogTextSubstitutions substitutions(errTitle, errMessage, errNumberString, PSTR(""));
//	CenterAlert(rDeathAlertID);

	dummyInt = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(rDeathAlertID, &substitutions);
	PLSysCalls::Exit(-1);
}

//--------------------------------------------------------------  CreateOffScreenBitMap
// Creates an offscreen bit map (b&w - 1 bit depth).

/*
void CreateOffScreenBitMap (Rect *theRect, GrafPtr *offScreen)
{
	GrafPtr		theBWPort;
	BitMap		theBitMap;
	long		theRowBytes;

	theBWPort = (GrafPtr)(NewPtr(sizeof(GrafPort)));
	OpenPort(theBWPort);
	theRowBytes = (long)((theRect->right - theRect->left + 15L) / 16L) * 2L;
	theBitMap.rowBytes = (short)theRowBytes;
	theBitMap.baseAddr = NewPtr((long)theBitMap.rowBytes *
		(theRect->bottom - theRect->top));
	if (theBitMap.baseAddr == nil)
		RedAlert(kErrNoMemory);
	theBitMap.bounds = *theRect;
	if (MemError() != PLErrors::kNone)
		RedAlert(kErrNoMemory);
	SetPortBits(&theBitMap);
	ClipRect(theRect);
	RectRgn(theBWPort->visRgn, theRect);
	EraseRect(theRect);
	*offScreen = theBWPort;
}
*/
//--------------------------------------------------------------  CreateOffScreenPixMap
// Creates an offscreen pix map using the depth of the current device.
/*
void CreateOffScreenPixMap (Rect *theRect, CGrafPtr *offScreen)
{
	CTabHandle	thisColorTable;
	GDHandle	oldDevice;
	CGrafPtr	newCGrafPtr;
	Ptr			theseBits;
	long		sizeOfOff, offRowBytes;
	PLError_t		theErr;
	short		thisDepth;
	char		wasState;

	oldDevice = GetGDevice();
	SetGDevice(thisGDevice);
	newCGrafPtr = nil;
	newCGrafPtr = (CGrafPtr)NewPtr(sizeof(CGrafPort));
	if (newCGrafPtr != nil)
	{
		OpenCPort(newCGrafPtr);
		thisDepth = (**(*newCGrafPtr).portPixMap).pixelSize;
		offRowBytes = ((((long)thisDepth *
				(long)(theRect->right - theRect->left)) + 15L) >> 4L) << 1L;
		sizeOfOff = (long)(theRect->bottom - theRect->top + 1) * offRowBytes;
	//	sizeOfOff = (long)(theRect->bottom - theRect->top) * offRowBytes;
		OffsetRect(theRect, -theRect->left, -theRect->top);
		theseBits = NewPtr(sizeOfOff);
		if (theseBits != nil)
		{								// workaround
			(**(*newCGrafPtr).portPixMap).baseAddr = theseBits + offRowBytes;
		//	(**(*newCGrafPtr).portPixMap).baseAddr = theseBits;
			(**(*newCGrafPtr).portPixMap).rowBytes = (short)offRowBytes + 0x8000;
			(**(*newCGrafPtr).portPixMap).bounds = *theRect;
			wasState = HGetState((Handle)thisGDevice);
			HLock((Handle)thisGDevice);
			thisColorTable = (**(**thisGDevice).gdPMap).pmTable;
			HSetState((Handle)thisGDevice, wasState);
			theErr = HandToHand((Handle *)&thisColorTable);
			(**(*newCGrafPtr).portPixMap).pmTable = thisColorTable;
			ClipRect(theRect);
			RectRgn(newCGrafPtr->visRgn, theRect);
			ForeColor(blackColor);
			BackColor(whiteColor);
			EraseRect(theRect);
		}
		else
		{
			CloseCPort(newCGrafPtr);
			DisposePtr((Ptr)newCGrafPtr);
			newCGrafPtr = nil;
			RedAlert(kErrNoMemory);
		}
	}
	else
		RedAlert(kErrNoMemory);

	*offScreen = newCGrafPtr;
	SetGDevice(oldDevice);
}
*/
//--------------------------------------------------------------------  CreateOffScreenGWorld
// Creates an offscreen GWorld�using the depth passed in.

PLError_t CreateOffScreenGWorld (DrawSurface **theGWorld, Rect *bounds)
{
	GpPixelFormat_t pixelFormat = PortabilityLayer::DisplayDeviceManager::GetInstance()->GetPixelFormat();

	return NewGWorld(theGWorld, pixelFormat, bounds, nil);
}


PLError_t CreateOffScreenGWorldCustomDepth(DrawSurface **theGWorld, Rect *bounds, GpPixelFormat_t pixelFormat)
{
	return NewGWorld(theGWorld, pixelFormat, bounds, nil);
}

//--------------------------------------------------------------  KillOffScreenPixMap
// Destroys memory allocated by an offscreen pix map.
/*
void KillOffScreenPixMap (CGrafPtr offScreen)
{
	Ptr		theseBits;

	if (offScreen != nil)
	{
		theseBits = (**(*offScreen).portPixMap).baseAddr;
		theseBits -= (**(*offScreen).portPixMap).rowBytes & 0x7FFF;	// workaround
		DisposePtr(theseBits);
		DisposeHandle((Handle)(**(*offScreen).portPixMap).pmTable);
		CloseCPort(offScreen);
		DisposePtr((Ptr)offScreen);
	}
}
*/
//--------------------------------------------------------------  KillOffScreenBitMap
// Destroys memory allocated by an offscreen bit map.
/*
void KillOffScreenBitMap (GrafPtr offScreen)
{
	if (offScreen != nil)
	{
		DisposePtr((Ptr)(offScreen->portBits.baseAddr));
		ClosePort(offScreen);
		DisposePtr((Ptr)offScreen);
	}
}
*/
//--------------------------------------------------------------  LoadGraphic
// Function loads the specified 'PICT' from disk and draws it to�
// the current port (no scaling, clipping, etc, are done).  Always�
// draws in the upper left corner of current port.

void LoadGraphic (DrawSurface *surface, short resID)
{
	Rect					bounds;
	THandle<BitmapImage>	thePicture;

	thePicture = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', resID).StaticCast<BitmapImage>();
	if (thePicture == nil)
		RedAlert(kErrFailedGraphicLoad);

	bounds = (*thePicture)->GetRect();
	OffsetRect(&bounds, -bounds.left, -bounds.top);
	surface->DrawPicture(thePicture, bounds);

	thePicture.Dispose();
}

void LoadGraphicNoDither(DrawSurface *surface, short resID)
{
	Rect					bounds;
	THandle<BitmapImage>	thePicture;

	thePicture = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', resID).StaticCast<BitmapImage>();
	if (thePicture == nil)
		RedAlert(kErrFailedGraphicLoad);

	bounds = (*thePicture)->GetRect();
	OffsetRect(&bounds, -bounds.left, -bounds.top);
	surface->DrawPicture(thePicture, bounds, false);

	thePicture.Dispose();
}

//--------------------------------------------------------------  LoadGraphicCustom
// Same as LoadGraphic but supports custom graphics
void LoadGraphicCustom(DrawSurface *surface, short resID)
{
	Rect					bounds;
	THandle<BitmapImage>	thePicture;

	thePicture = LoadHouseResource('PICT', resID).StaticCast<BitmapImage>();
	if (thePicture == nil)
		RedAlert(kErrFailedGraphicLoad);

	bounds = (*thePicture)->GetRect();
	OffsetRect(&bounds, -bounds.left, -bounds.top);
	surface->DrawPicture(thePicture, bounds);

	thePicture.Dispose();
}

//--------------------------------------------------------------  LoadScaledGraphic
// Loads the specified 'PICT' and draws it mapped to the rectangle�
// specified.  If this rect isn't the same size of the 'PICT', scaling�
// will occur.

void LoadScaledGraphic (DrawSurface *surface, short resID, Rect *theRect)
{
	THandle<BitmapImage>	thePicture;

	thePicture = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', resID).StaticCast<BitmapImage>();
	if (thePicture == nil)
		RedAlert(kErrFailedGraphicLoad);
	surface->DrawPicture(thePicture, *theRect);
	thePicture.Dispose();
}

//--------------------------------------------------------------  LoadScaledGraphic
// Loads the specified 'PICT' and draws it mapped to the rectangle�
// specified.  If this rect isn't the same size of the 'PICT', scaling�
// will occur.

void LoadScaledGraphicCustom(DrawSurface *surface, short resID, Rect *theRect)
{
	THandle<BitmapImage>	thePicture;

	thePicture = LoadHouseResource('PICT', resID).StaticCast<BitmapImage>();
	if (thePicture == nil)
		RedAlert(kErrFailedGraphicLoad);
	surface->DrawPicture(thePicture, *theRect);
	thePicture.Dispose();
}

//--------------------------------------------------------------  LargeIconPlot
// Draws a standard b&w icon (32 x 32) - resource is an 'ICON'.

bool LargeIconPlot (DrawSurface *surface, PortabilityLayer::IResourceArchive *resFile, short resID, const Rect &theRect)
{
	Handle hdl = resFile->LoadResource('icl8', resID);
	if (hdl)
	{
		THandle<PortabilityLayer::PixMapImpl> img = PortabilityLayer::IconLoader::GetInstance()->LoadSimpleColorIcon(hdl);

		if (img)
		{
			CopyBits(*img, *surface->m_port.GetPixMap(), &(*img)->m_rect, &theRect, srcCopy);
			img.Dispose();
		}

		hdl.Dispose();
		return true;
	}

	hdl = resFile->LoadResource('ICN#', resID);
	if (hdl)
	{
		THandle<PortabilityLayer::PixMapImpl> img = PortabilityLayer::IconLoader::GetInstance()->LoadBWIcon(hdl);

		if (img)
		{
			CopyBits(*img, *surface->m_port.GetPixMap(), &(*img)->m_rect, &theRect, srcCopy);
			img.Dispose();
		}

		hdl.Dispose();
		return true;
	}

	return false;
}

//--------------------------------------------------------------  DrawCIcon

// Draws a standard color icon (32 x 32) - resource is a 'CICN'.

void DrawCIcon (DrawSurface *surface, short theID, short h, short v)
{
	THandle<PortabilityLayer::PixMapImpl> colorImage;
	THandle<PortabilityLayer::PixMapImpl> bwImage;
	THandle<PortabilityLayer::PixMapImpl> maskImage;

	if (PortabilityLayer::IconLoader::GetInstance()->LoadColorIcon(theID, colorImage, bwImage, maskImage))
	{
		Rect		theRect;

		SetRect(&theRect, 0, 0, 32, 32);
		OffsetRect(&theRect, h, v);

		CopyMask(*colorImage, *maskImage, *surface->m_port.GetPixMap(), &(*colorImage)->m_rect, &(*maskImage)->m_rect, &theRect);
		surface->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

		bwImage.Dispose();
		colorImage.Dispose();
		maskImage.Dispose();
	}
}

//--------------------------------------------------------------  LongSquareRoot

// This is a quick and dirty square root function that returns pretty�
// accurate long integer results.  It uses no transcendental functions or�
// floating point.

long LongSquareRoot (long theNumber)
{
	long		currentAnswer;
	long		nextTrial;

	if (theNumber <= 1L)
		return (theNumber);

	nextTrial = theNumber / 2;

	do
	{
		currentAnswer = nextTrial;
		nextTrial = (nextTrial + theNumber / nextTrial) / 2;
	}
	while (nextTrial < currentAnswer);

	return(currentAnswer);
}

//--------------------------------------------------------------  WaitForInputEvent

// Wait for either a key to be hit or the mouse button to be clicked.
// Also has a "timeout" parameter ("seconds").

Boolean WaitForInputEvent (short seconds)
{
	TimeTaggedVOSEvent	theEvent;
	long				timeToBail;
	Boolean				waiting, didResume;

	timeToBail = TickCount() + 60L * (long)seconds;
	FlushEvents();
	waiting = true;
	didResume = false;


	while (waiting)
	{
		const KeyDownStates *theKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

		if (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kControl)) || theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kAlt)) || theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kShift)))
			waiting = false;

		if (PortabilityLayer::EventQueue::GetInstance()->Dequeue(&theEvent))
		{
			if (theEvent.IsLMouseDownEvent() || theEvent.IsKeyDownEvent())
				waiting = false;
		}
		if ((seconds != -1) && (TickCount() >= timeToBail))
			waiting = false;

		Delay(1, nullptr);
	}
	FlushEvents();
	return (didResume);
}

//--------------------------------------------------------------  WaitCommandQReleased

// Waits until the Command-Q key combination is released.

void WaitCommandQReleased (void)
{
	Boolean		waiting;

	waiting = true;

	while (waiting)
	{
		const KeyDownStates *theKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

		if (!theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kControl)) || !theKeys->IsSet(PL_KEY_ASCII('Q')))
			waiting = false;

		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		Delay(1, nullptr);
	}
	FlushEvents();
}

//--------------------------------------------------------------  KeyMapOffsetFromRawKey
// Converts a raw key code to keymap offset (ugly stuff).

char KeyMapOffsetFromRawKey (char rawKeyCode)
{
	char		hiByte, loByte, theOffset;

	hiByte = rawKeyCode & 0xF0;
	loByte = rawKeyCode & 0x0F;

	if (loByte <= 0x07)
		theOffset = hiByte + (0x07 - loByte);
	else
		theOffset = hiByte + (0x17 - loByte);

	return (theOffset);
}

//--------------------------------------------------------------  GetKeyName
// Given a keyDown event (it's message field), this function returns�
// a string with that key's name (so we get "Shift" and "Esc", etc.).

static const char *gs_specialKeyNames[GpKeySpecials::kCount] =
{
	"tab",
	"esc",
	"prnt scrn",
	"scrl lock",
	"pause",
	"insert",
	"home",
	"page up",
	"page dn",
	"delete",
	"end",
	"backspace",
	"caps lock",
	"enter",
	"lf shift",
	"rt shift",
	"lf ctrl",
	"rt ctrl",
	"lf alt",
	"rt alt",
	"num lock",
	"lf arrow",
	"up arrow",
	"dn arrow",
	"rt arrow"
};

static const char *gs_numPadSpecialKeyNames[GpNumPadSpecials::kCount] =
{
	"numpad /",
	"numpad *",
	"numpad -",
	"numpad +"
};

void GetKeyName (intptr_t message, StringPtr theName)
{
	KeyEventType eventType = PL_KEY_GET_EVENT_TYPE(message);
	const int eventValue = PL_KEY_GET_VALUE(message);
	const char *name1 = nullptr;
	const char *name2 = nullptr;

	char asciiNameBuffer[2];
	char fkeyNameBuffer[5];

	switch (eventType)
	{
	case KeyEventType_Special:
		name1 = gs_specialKeyNames[eventValue];
		break;
	case KeyEventType_ASCII:
		asciiNameBuffer[0] = static_cast<char>(eventValue);
		asciiNameBuffer[1] = '\0';
		name1 = asciiNameBuffer;

		if (eventValue == ' ')
			name1 = "space";
		break;
	case KeyEventType_MacRoman:
		PL_NotYetImplemented_TODO("UnicodeNames");
		break;
	case KeyEventType_NumPadNumber:
		asciiNameBuffer[0] = static_cast<char>('0' + eventValue);
		asciiNameBuffer[1] = '\0';
		name1 = "NumPad ";
		name2 = asciiNameBuffer;
		break;
	case KeyEventType_NumPadSpecial:
		name1 = "NumPad ";
		name2 = gs_numPadSpecialKeyNames[eventValue];
		break;
	case KeyEventType_FKey:
		fkeyNameBuffer[0] = 'F';
		if (eventValue >= 10)
		{
			fkeyNameBuffer[1] = static_cast<char>('0' + eventValue / 10);
			fkeyNameBuffer[2] = static_cast<char>('0' + eventValue % 10);
			fkeyNameBuffer[3] = '\0';
		}
		else
		{
			fkeyNameBuffer[1] = static_cast<char>('0' + eventValue);
			fkeyNameBuffer[2] = '\0';
		}

		name1 = fkeyNameBuffer;
	case KeyEventType_GamepadButton:
		// This should never happen
		assert(false);
		break;
	default:
		break;
	}

	const size_t name1Length = (name1 == nullptr) ? 0 : strlen(name1);
	const size_t name2Length = (name2 == nullptr) ? 0 : strlen(name2);

	const size_t combinedLength = name1Length + name2Length;
	assert(combinedLength <= 255);

	theName[0] = static_cast<uint8_t>(combinedLength);
	if (name1Length)
		memcpy(theName + 1, name1, name1Length);
	if (name2Length)
		memcpy(theName + 1 + name1Length, name2, name2Length);
}

//--------------------------------------------------------------  OptionKeyDown
// Returns true is the Option key is being held down.

Boolean OptionKeyDown (void)
{
	const KeyDownStates *theKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

	if (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kAlt)))
		return (true);
	else
		return (false);
}

//--------------------------------------------------------------  ExtractCTSeed
// Very esoteric - gets the "color table seed" from a specified graf port.
/*
long ExtractCTSeed (CGrafPtr porter)
{
	long		theSeed;

	theSeed = (**((**(porter->portPixMap)).pmTable)).ctSeed;
	return(theSeed);
}
*/
//--------------------------------------------------------------  ForceCTSeed
// Forces  the "color table seed" from a specified graf port to a�
// specified value.
/*
void ForceCTSeed (CGrafPtr porter, long newSeed)
{
	(**((**(porter->portPixMap)).pmTable)).ctSeed = newSeed;
}
*/
//--------------------------------------------------------------  DelayTicks
// Lil' function that just sits and waits a specified number of�
// Ticks (1/60 of a second).

void DelayTicks (long howLong)
{
	UInt32		whoCares;

	Delay(howLong, &whoCares);
}

//--------------------------------------------------------------  UnivGetSoundVolume
// Returns the speaker volume (as set by the user) in the range of�
// zero to seven (handles Sound Manager 3 case as well).

void UnivGetSoundVolume (short *volume, Boolean hasSM3)
{
	long		longVol;
	PLError_t		theErr;

//	if (hasSM3)
//	{
		longVol = PortabilityLayer::SoundSystem::GetInstance()->GetVolume();
		*volume = longVol / 0x0024;
//	}
//	else
//		GetSoundVol(volume);

	if (*volume > 7)
		*volume = 7;
	else if (*volume < 0)
		*volume = 0;
}

//--------------------------------------------------------------  UnivSetSoundVolume
// Sets the speaker volume to a specified value (in the range of�
// zero to seven (handles Sound Manager 3 case as well).

void  UnivSetSoundVolume (short volume, Boolean hasSM3)
{
	long		longVol;

	if (volume > 7)
		volume = 7;
	else if (volume < 0)
		volume = 0;

//	if (hasSM3)
//	{
		longVol = (long)volume * 0x0025;
		if (longVol > 0xff)
			longVol = 0xff;

		PortabilityLayer::SoundSystem::GetInstance()->SetVolume(longVol);
//	}
//	else
//		SetSoundVol(volume);
}

