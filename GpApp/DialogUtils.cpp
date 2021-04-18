//============================================================================
//----------------------------------------------------------------------------
//								DialogUtils.c
//----------------------------------------------------------------------------
//============================================================================

#include "DialogManager.h"
#include "PLArrayView.h"
#include "PLButtonWidget.h"
#include "PLControlDefinitions.h"
#include "PLNumberFormatting.h"
#include "PLPasStr.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"
#include "PLWidgets.h"
#include "QDStandardPalette.h"
#include "DialogUtils.h"
#include "Externs.h"
#include "FontFamily.h"
#include "ResourceManager.h"
#include "ResolveCachingColor.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"

#include <algorithm>

#define kActive						0
#define kInactive					255


//==============================================================  Functions
//--------------------------------------------------------------  BringUpDialog
// Given a dialog pointer and a resource ID, this function brings it up…
// centered, visible, and with the default button outlined.

void BringUpDialog (Dialog **theDialog, short dialogID, const DialogTextSubstitutions *substitutions)
{
	*theDialog = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(dialogID, kPutInFront, substitutions);

//	CenterDialog(dialogID);
	if (*theDialog == nil)
		RedAlert(kErrDialogDidntLoad);
	ShowWindow((*theDialog)->GetWindow());
	DrawDefaultButton(*theDialog);
}

//--------------------------------------------------------------  GetPutDialogCorner
// Determines the upper left corner coordinates needed to properly center…
// the standard Mac PutFile dialog (when you save files).
/*
void GetPutDialogCorner (Point *theCorner)
{
	DialogTHndl	dlogHandle;
	Rect		theScreen, dlogBounds;
	Byte		wasState;
	
	theCorner->h = 64;
	theCorner->v = 64;
	theScreen = qd.screenBits.bounds;
	theScreen.top += LMGetMBarHeight();
	OffsetRect(&theScreen, -theScreen.left, -theScreen.top);
	
	dlogHandle = (DialogTHndl)GetResource('DLOG', sfPutDialogID);
	if (dlogHandle != nil)
	{
		wasState = HGetState((Handle)dlogHandle);
		HLock((Handle)dlogHandle);
		
		dlogBounds = (**dlogHandle).boundsRect;
		OffsetRect(&dlogBounds, -dlogBounds.left, -dlogBounds.top);
		
		theCorner->h = (theScreen.right - dlogBounds.right) / 2;
		theCorner->v = (theScreen.bottom - dlogBounds.bottom) / 3;
		
		HSetState((Handle)dlogHandle, wasState);
	}
	theCorner->v += LMGetMBarHeight();
}
*/

//--------------------------------------------------------------  GetPutDialogCorner
// Determines the upper left corner coordinates needed to properly center…
// the standard Mac GetFile dialog (when you open files).
/*
void GetGetDialogCorner (Point *theCorner)
{
	DialogTHndl	dlogHandle;
	Rect		theScreen, dlogBounds;
	Byte		wasState;
	
	theCorner->h = 64;
	theCorner->v = 64;
	theScreen = qd.screenBits.bounds;
	theScreen.top += LMGetMBarHeight();
	OffsetRect(&theScreen, -theScreen.left, -theScreen.top);
	
	dlogHandle = (DialogTHndl)GetResource('DLOG', sfGetDialogID);
	if (dlogHandle != nil)
	{
		wasState = HGetState((Handle)dlogHandle);
		HLock((Handle)dlogHandle);
		
		dlogBounds = (**dlogHandle).boundsRect;
		OffsetRect(&dlogBounds, -dlogBounds.left, -dlogBounds.top);
		
		theCorner->h = (theScreen.right - dlogBounds.right) / 2;
		theCorner->v = (theScreen.bottom - dlogBounds.bottom) / 3;
		
		HSetState((Handle)dlogHandle, wasState);
	}
	theCorner->v += LMGetMBarHeight();
}
*/
//--------------------------------------------------------------  CenterDialog
// Given a resource ID for a dialog, this function properly centers it.
/*
void CenterDialog (SInt16 dialogID)
{
	DialogTHndl	dlogHandle;
	Rect		theScreen, dlogBounds;
	SInt16		hPos, vPos;
	Byte		wasState;
	
	theScreen = qd.screenBits.bounds;
	theScreen.top += LMGetMBarHeight();
	
	dlogHandle = (DialogTHndl)GetResource('DLOG', dialogID);
	if (dlogHandle != nil)
	{
		wasState = HGetState((Handle)dlogHandle);
		HLock((Handle)dlogHandle);
		
		dlogBounds = (**dlogHandle).boundsRect;
		OffsetRect(&dlogBounds, -dlogBounds.left, -dlogBounds.top);
		
		hPos = ((theScreen.right - theScreen.left) - dlogBounds.right) / 2;
		vPos = ((theScreen.bottom - theScreen.top) - dlogBounds.bottom) / 3;
		
		OffsetRect(&dlogBounds, hPos, vPos + LMGetMBarHeight());
		
		(**dlogHandle).boundsRect = dlogBounds;
		HSetState((Handle)dlogHandle, wasState);
	}
}
*/
//--------------------------------------------------------------  GetDialogRect
// Determines the bounding rectangle for a given dialog.

void GetDialogRect (Rect *bounds, short dialogID)
{
	Handle dlogHandle = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('DLOG', dialogID).StaticCast<void>();
	if (dlogHandle != nil)
	{
		BERect dataRect = **dlogHandle.StaticCast<BERect>();
		*bounds = dataRect.ToRect();

		dlogHandle.Dispose();
	}
}

//--------------------------------------------------------------  TrueCenterDialog
// Places a dialog DEAD CENTER (as opposed to 1/3 of the way down as…
// is common for Mac dialog centering).
/*
void TrueCenterDialog (short dialogID)
{
	DialogTHndl	dlogHandle;
	Rect		theScreen, dlogBounds;
	short		hPos, vPos;
	Byte		wasState;
	
	theScreen = qd.screenBits.bounds;
	theScreen.top += LMGetMBarHeight();
	
	dlogHandle = (DialogTHndl)GetResource('DLOG', dialogID);
	if (dlogHandle != nil)
	{
		wasState = HGetState((Handle)dlogHandle);
		HLock((Handle)dlogHandle);
		
		dlogBounds = (**dlogHandle).boundsRect;
		OffsetRect(&dlogBounds, theScreen.left - dlogBounds.left, 
				theScreen.top - dlogBounds.top);
		
		hPos = ((theScreen.right - theScreen.left) - 
				(dlogBounds.right - dlogBounds.left)) / 2;
		vPos = ((theScreen.bottom - theScreen.top) - 
				(dlogBounds.bottom - dlogBounds.top)) / 2;
		
		OffsetRect(&dlogBounds, hPos, vPos + LMGetMBarHeight());
		
		(**dlogHandle).boundsRect = dlogBounds;
		HSetState((Handle)dlogHandle, wasState);
	}
}
*/
//--------------------------------------------------------------  CenterAlert
// Given an alert ID, this function properly centers it on the main monitor.
/*
void CenterAlert (short alertID)
{
	AlertTHndl	alertHandle;
	Rect		theScreen, alertRect;
	short		horiOff, vertOff;
	Byte		wasState;
	
	theScreen = qd.screenBits.bounds;
	theScreen.top += LMGetMBarHeight();
	
	alertHandle = (AlertTHndl)GetResource('ALRT', alertID);
	if (alertHandle != nil)
	{
		wasState = HGetState((Handle)alertHandle);
		HLock((Handle)alertHandle);
		
		alertRect = (**alertHandle).boundsRect;
		OffsetRect(&alertRect, -alertRect.left, -alertRect.top);
		
		horiOff = ((theScreen.right - theScreen.left) - alertRect.right) / 2;	
		vertOff = ((theScreen.bottom - theScreen.top) - alertRect.bottom) / 3;
		
		OffsetRect(&alertRect, horiOff, vertOff + LMGetMBarHeight());
		
		(**alertHandle).boundsRect = alertRect;
		HSetState((Handle)alertHandle, wasState);
	}
}
*/
//--------------------------------------------------------------  ZoomOutDialogRect

// Given a dialog, this function does the "zoom" animation to make the…
// the dialog appear to expand from nothingness or zoom in at you.
/*
void ZoomOutDialogRect (short dialogID)
{
	#define		kSteps		16
	#define		kZoomDelay	1
	DialogTHndl	dlogHandle;
	GrafPtr		wasPort, tempPort;
	Rect		dlogBounds, zoomRect;
	UInt32		dummyLong;
	Byte		wasState;
	short		wideStep, highStep, i;
	
	GetPort(&wasPort);
	
	tempPort = (GrafPtr)NewPtrClear(sizeof(GrafPort));
	OpenPort(tempPort);
	
	dlogHandle = (DialogTHndl)GetResource('DLOG', dialogID);
	if (dlogHandle != nil)
	{
		wasState = HGetState((Handle)dlogHandle);
		HLock((Handle)dlogHandle);
		dlogBounds = (**dlogHandle).boundsRect;
		HSetState((Handle)dlogHandle, wasState);
	}
	
	wideStep = ((dlogBounds.right - dlogBounds.left) / 2) / kSteps;
	highStep = ((dlogBounds.bottom - dlogBounds.top) / 2) / kSteps;
	
	SetRect(&zoomRect, dlogBounds.left + (wideStep * kSteps), 
			dlogBounds.top + (highStep * kSteps), 
			dlogBounds.right - (wideStep * kSteps), 
			dlogBounds.bottom - (highStep * kSteps));
	GlobalToLocalRect(&zoomRect);
	
	PenPat(GetQDGlobalsGray(&dummyPattern));
	PenMode(patXor);
	
	for (i = 0; i < kSteps; i++)
	{
		FrameRect(&zoomRect);
		Delay(kZoomDelay, &dummyLong);
		FrameRect(&zoomRect);
		InsetRect(&zoomRect, -wideStep, -highStep);
	}
	
	ClosePort(tempPort);
	
	SetPort((GrafPtr)wasPort);
}
*/
//--------------------------------------------------------------  ZoomOutAlertRect

// Like the above funciton but zooms out alerts instead of dialogs.
/*
void ZoomOutAlertRect (short alertID)
{
	#define		kSteps		16
	#define		kZoomDelay	1
	AlertTHndl	alertHandle;
	GrafPtr		wasPort, tempPort;
	Rect		alertBounds, zoomRect;
	UInt32		dummyLong;
	Byte		wasState;
	short		wideStep, highStep, i;
	
	GetPort(&wasPort);
	
	tempPort = (GrafPtr)NewPtrClear(sizeof(GrafPort));
	OpenPort(tempPort);
	
	alertHandle = (AlertTHndl)GetResource('ALRT', alertID);
	if (alertHandle != nil)
	{
		wasState = HGetState((Handle)alertHandle);
		HLock((Handle)alertHandle);
		alertBounds = (**alertHandle).boundsRect;
		HSetState((Handle)alertHandle, wasState);
	}
	
	wideStep = ((alertBounds.right - alertBounds.left) / 2) / kSteps;
	highStep = ((alertBounds.bottom - alertBounds.top) / 2) / kSteps;
	
	SetRect(&zoomRect, alertBounds.left + (wideStep * kSteps), 
			alertBounds.top + (highStep * kSteps), 
			alertBounds.right - (wideStep * kSteps), 
			alertBounds.bottom - (highStep * kSteps));
	GlobalToLocalRect(&zoomRect);
	
	PenPat(GetQDGlobalsGray(&dummyPattern));
	PenMode(patXor);
	
	for (i = 0; i < kSteps; i++)
	{
		FrameRect(&zoomRect);
		Delay(kZoomDelay, &dummyLong);
		FrameRect(&zoomRect);
		InsetRect(&zoomRect, -wideStep, -highStep);
	}
	
	ClosePort(tempPort);
	
	SetPort((GrafPtr)wasPort);
}
*/

//--------------------------------------------------------------  FlashDialogButton
// Flashes the default dialog button (item = 1) so as to make it appear…
// as though the user clicked on it.

void FlashDialogButton (Dialog *theDialog, short itemNumber)
{
	PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();

	ControlHandle	itemHandle;
	UInt32			dummyLong;

	PortabilityLayer::Widget *widget = theDialog->GetItems()[itemNumber - 1].GetWidget();

	widget->SetHighlightStyle(kControlButtonPart, true);
	Delay(8, &dummyLong);
	widget->SetHighlightStyle(kControlButtonPart, false);
}

//--------------------------------------------------------------  DrawDefaultButton
// Draws a fat outline around the default item (item = 1).  This is the…
// item that is selected if the user hits the Return key.

void DrawDefaultButton (Dialog *theDialog)
{
	const PortabilityLayer::DialogItem	&firstItem = *theDialog->GetItems().begin();
	Rect								itemRect = firstItem.GetWidget()->GetRect();
	DrawSurface							*surface = theDialog->GetWindow()->GetDrawSurface();

	PortabilityLayer::ButtonWidget::DrawDefaultButtonChrome(itemRect, surface);
}

//--------------------------------------------------------------  GetDialogString
// Returns a string from a specific dialog item.

void GetDialogString (Dialog *theDialog, short item, StringPtr theString)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	const PLPasStr str = itemRef.GetWidget()->GetString();

	const uint8_t length = static_cast<uint8_t>(std::min<size_t>(255, str.Length()));

	theString[0] = length;
	memcpy(theString + 1, str.UChars(), length);
}

//--------------------------------------------------------------  SetDialogString
// Sets a specific string to a specific dialog item.

void SetDialogString (Dialog *theDialog, short item, const PLPasStr &theString)
{
	PortabilityLayer::Widget *widget = theDialog->GetItems()[item - 1].GetWidget();
	widget->SetString(theString);
	widget->DrawControl(theDialog->GetWindow()->GetDrawSurface());
}

//--------------------------------------------------------------  GetDialogStringLen
// Returns the length of a dialog item string (text).

short GetDialogStringLen (Dialog *theDialog, short item)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	return itemRef.GetWidget()->GetString().Length();
}

//--------------------------------------------------------------  GetDialogItemValue
// Returns the value or "state" of a dialog item.  For checkboxes and…
// radio buttons, this may be a 1 or 0.

void GetDialogItemValue (Dialog *theDialog, short item, short *theState)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	*theState = itemRef.GetWidget()->GetState();
}

//--------------------------------------------------------------  SetDialogItemValue
// Sets a specific dialogf items value or state (can set or clear…
// checkboxes, radio buttons, etc.).

void SetDialogItemValue (Dialog *theDialog, short item, short theState)
{
	theDialog->GetItems()[item - 1].GetWidget()->SetState(theState);
}

//--------------------------------------------------------------  ToggleDialogItemValue
// If item is a checkbox or radio button, its state is toggled.

void ToggleDialogItemValue (Dialog *theDialog, short item)
{
	int16_t			theState;

	PortabilityLayer::Widget *widget = theDialog->GetItems()[item - 1].GetWidget();

	theState = widget->GetState();
	if (theState == 0)
		theState = 1;
	else
		theState = 0;
	widget->SetState(theState);
	widget->DrawControl(theDialog->GetWindow()->GetDrawSurface());
}

//--------------------------------------------------------------  SetDialogNumToStr
// Function accepts an integer, converts it to a string and sets a…
// dialog items text to this string.

void SetDialogNumToStr (Dialog *theDialog, short item, long theNumber)
{
	Str255			theString;
	ControlHandle	itemHandle;

	NumToString(theNumber, theString);
	PortabilityLayer::Widget *widget = theDialog->GetItems()[item - 1].GetWidget();
	widget->SetString(theString);
	widget->DrawControl(theDialog->GetWindow()->GetDrawSurface());
}

//--------------------------------------------------------------  GetDialogNumFromStr
// Function extracts the text from a dialog item and converts it to an…
// integer for returning.

void GetDialogNumFromStr (Dialog *theDialog, short item, long *theNumber)
{
	Str255			theString;
	GetDialogString(theDialog, item, theString);
	StringToNum(theString, theNumber);
}

//--------------------------------------------------------------  GetDialogItemRect
// Returns the bounding rectangle of the specified dialog item.

void GetDialogItemRect (Dialog *theDialog, short item, Rect *theRect)
{
	*theRect = theDialog->GetItems()[item - 1].GetWidget()->GetRect();
}

//--------------------------------------------------------------  SetDialogItemRect
// Sets the bounding rectangle of the specified dialog item.  Used to…
// resize or move a control.

void SetDialogItemRect (Dialog *theDialog, short item, Rect *theRect)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();
	widget->SetPosition(Point::Create(theRect->left, theRect->top));
	widget->Resize(theRect->Width(), theRect->Height());
}

//--------------------------------------------------------------  OffsetDialogItemRect
// Moves a dialog item by h and v.

void OffsetDialogItemRect (Dialog *theDialog, short item, short h, short v)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();

	const Rect oldRect = widget->GetRect();
	widget->SetPosition(Point::Create(oldRect.left + h, oldRect.top + v));
}

//--------------------------------------------------------------  SelectFromRadioGroup
// Assuming a series of consecutively numbered radio buttons, this function…
// clears the whole range of them but sets the one specified (as though…
// the radio buttons are linked and only one can be set at a time).

void SelectFromRadioGroup (Dialog *dial, short which, short first, short last)
{
	for (int i = first; i <= last; i++)
		dial->GetItems()[i - 1].GetWidget()->SetState(0);
	
	dial->GetItems()[which - 1].GetWidget()->SetState(1);
}

//--------------------------------------------------------------  AddMenuToPopUp
// Assigns a menu handle to a pop-up dialog item - thus, giving that…
// pop-up item something to pop up.
/*
void AddMenuToPopUp (Dialog *theDialog, short whichItem, MenuHandle theMenu)
{
	Rect		iRect;
	Handle		iHandle;
	short		iType;
	
	GetDialogItem(theDialog, whichItem, &iType, &iHandle, &iRect);
	(**(ControlHandle)iHandle).contrlRfCon = (long)theMenu;
}
*/
//--------------------------------------------------------------  GetPopUpMenuValu
// Returns which item is currently selected in a pop-up menu.

void GetPopUpMenuValue (Dialog *theDialog, short whichItem, short *value)
{
	GetDialogItemValue(theDialog, whichItem, value);
}

//--------------------------------------------------------------  SetPopUpMenuValue
// Forces a specific item to be set (as though selected) in a pop-up menu.

void SetPopUpMenuValue (Dialog *theDialog, short whichItem, short value)
{
	SetDialogItemValue(theDialog, whichItem, value);
}

//--------------------------------------------------------------  MyEnableControl
// "Un-grays" or enables a dialog item (usually a button).

void MyEnableControl (Dialog *theDialog, short whichItem)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[whichItem - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();
	widget->SetEnabled(true);
}

//--------------------------------------------------------------  MyDisableControl
// "Grays out" or disables a dialog item (usually a button).

void MyDisableControl (Dialog *theDialog, short whichItem)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[whichItem - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();
	widget->SetEnabled(false);
}

//--------------------------------------------------------------  DrawDialogUserText
// Given a string of text and an item, this function draws the string…
// within the bounding rect of the item.  Dialog item assumed to be…
// a "user item" (invisible item with only bounds).

void DrawDialogUserText (Dialog *dial, short item, StringPtr text, Boolean invert)
{
	ControlHandle	iHandle;
	Str255			stringCopy;
	short			inset;

	DrawSurface *surface = dial->GetWindow()->GetDrawSurface();

	PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication9);
	
	PasStringCopy(text, stringCopy);

	Rect iRect = dial->GetItems()[item - 1].GetWidget()->GetRect();

	if ((appFont->MeasurePStr(stringCopy) + 2) > (iRect.right - iRect.left))
		CollapseStringToWidth(appFont, stringCopy, iRect.right - iRect.left - 2);
	
	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	surface->FillRect(iRect, whiteColor);

	short strWidth = appFont->MeasurePStr(stringCopy);
	inset = ((iRect.right - iRect.left) - (strWidth + 2)) / 2;
	iRect.left += inset;
	iRect.right -= inset;

	// Draw centered

	const int32_t ascender = appFont->GetMetrics().m_ascent;

	PortabilityLayer::ResolveCachingColor backgroundColor;
	PortabilityLayer::ResolveCachingColor textColor;

	if (invert)
	{
		backgroundColor = StdColors::Black();
		textColor = StdColors::White();
	}
	else
	{
		backgroundColor = StdColors::White();
		textColor = StdColors::Black();
	}

	surface->FillRect(iRect, backgroundColor);

	const Point centeredDrawPoint = Point::Create((iRect.left + iRect.right - strWidth) / 2, (iRect.top + iRect.bottom + ascender) / 2);
	surface->DrawString(centeredDrawPoint, stringCopy, textColor, appFont);
}

//--------------------------------------------------------------  DrawDialogUserText
// Similar to the above function but doesn't call TETextBox().  Instead,…
// it truncates the string (and appends "…") to the end in order that…
// the string fits within the dialog item's bounds.

void DrawDialogUserText2 (Dialog *dial, short item, StringPtr text)
{
	Str255			stringCopy;

	DrawSurface *surface = dial->GetWindow()->GetDrawSurface();
	PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication9);
	
	PasStringCopy(text, stringCopy);
	const Rect iRect = dial->GetItems()[item - 1].GetWidget()->GetRect();

	if ((appFont->MeasurePStr(stringCopy) + 2) > (iRect.right - iRect.left))
		CollapseStringToWidth(appFont, stringCopy, iRect.right - iRect.left - 2);

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	surface->DrawString(Point::Create(iRect.left, iRect.bottom), stringCopy, blackColor, appFont);
}

//--------------------------------------------------------------  LoadDialogPICT
// Draws a 'PICT' specified by ID within the bounds of the specified…
// dialog item.

void LoadDialogPICT (Dialog *theDialog, short item, short theID)
{
	THandle<BitmapImage>	thePict;

	Rect			iRect = theDialog->GetItems()[item - 1].GetWidget()->GetRect();;

	thePict = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', theID).StaticCast<BitmapImage>();
	if (thePict)
		theDialog->GetWindow()->GetDrawSurface()->DrawPicture(thePict, iRect);
}

//--------------------------------------------------------------  FrameDialogItem
// Given a dialog item, this function draws a box around it.

void FrameDialogItem (Dialog *theDialog, short item, PortabilityLayer::ResolveCachingColor &color)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();

	const Rect itemRect = widget->GetRect();

	theDialog->GetWindow()->GetDrawSurface()->FrameRect(itemRect, color);
}

//--------------------------------------------------------------  FrameDialogItemC
// Given a dialog item, this function draws a color (specified) box around it.

void FrameDialogItemC (Dialog *theDialog, short item, long color)
{
	DrawSurface		*surface = theDialog->GetWindow()->GetDrawSurface();
	const Rect		itemRect = theDialog->GetItems()[item - 1].GetWidget()->GetRect();

	PortabilityLayer::ResolveCachingColor frameColor = PortabilityLayer::ResolveCachingColor::FromStandardColor(color);

	surface->FrameRect(itemRect, frameColor);
}

//--------------------------------------------------------------  FrameOvalDialogItem
// Given a dialog item, this function draws an oval around it.

void FrameOvalDialogItem (Dialog *theDialog, short item, PortabilityLayer::ResolveCachingColor &color)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();

	const Rect itemRect = widget->GetRect();

	theDialog->GetWindow()->GetDrawSurface()->FrameEllipse(itemRect, color);
}

//--------------------------------------------------------------  BorderDialogItem
// Given a dialog item, this function draws any combination of 4 sides…
// of a box around it.  Which sides get drawn is encoded in "sides".

void BorderDialogItem(Dialog *theDialog, short item, short sides, short thickness, const uint8_t *pattern)
{
	// 1 = left
	// 2 = top
	// 4 = bottom
	// 8 = right ... so 6 = top & bottom, 15 = all 4 sides

	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();
	const Rect itemRect = widget->GetRect();

	DrawSurface *surface = theDialog->GetWindow()->GetDrawSurface();

	if (sides >= 8)				// 8 = right
	{
		const Point pointA = Point::Create(itemRect.right, itemRect.top);
		const Point pointB = Point::Create(itemRect.right + thickness, itemRect.bottom + 1);
		const Rect rect = Rect::Create(pointA.v, pointA.h, pointB.v, pointB.h);
		if (pattern)
		{
			surface->FillRect(rect, whiteColor);
			surface->FillRectWithMaskPattern8x8(rect, pattern,blackColor);
		}
		else
			surface->FillRect(rect, blackColor);
		sides -= 8;
	}
	if (sides >= 4)				// 4 = bottom
	{
		const Point pointA = Point::Create(itemRect.left, itemRect.bottom);
		const Point pointB = Point::Create(itemRect.right + 1, itemRect.bottom + thickness);
		const Rect rect = Rect::Create(pointA.v, pointA.h, pointB.v, pointB.h);
		if (pattern)
		{
			surface->FillRect(rect, whiteColor);
			surface->FillRectWithMaskPattern8x8(rect, pattern, blackColor);
		}
		else
			surface->FillRect(rect, blackColor);
		sides -= 4;
	}
	if (sides >= 2)				// 2 = top
	{
		const Point pointA = Point::Create(itemRect.left, itemRect.top - 1);
		const Point pointB = Point::Create(itemRect.right + 1, itemRect.top + thickness);
		const Rect rect = Rect::Create(pointA.v, pointA.h, pointB.v, pointB.h);
		if (pattern)
		{
			surface->FillRect(rect, whiteColor);
			surface->FillRectWithMaskPattern8x8(rect, pattern, blackColor);
		}
		else
			surface->FillRect(rect, blackColor);
		sides -= 2;
	}
	if (sides >= 1)				// 1 = left
	{
		const Point pointA = Point::Create(itemRect.left - 1, itemRect.top);
		const Point pointB = Point::Create(itemRect.left - 1 + thickness, itemRect.bottom + 1);
		const Rect rect = Rect::Create(pointA.v, pointA.h, pointB.v, pointB.h);
		if (pattern)
		{
			surface->FillRect(rect, whiteColor);
			surface->FillRectWithMaskPattern8x8(rect, pattern, blackColor);
		}
		else
			surface->FillRect(rect, blackColor);
	}
}

//--------------------------------------------------------------  ShadowDialogItem
// Draws a drop shadow to the right and below a specified dialog item.

void ShadowDialogItem (Dialog *theDialog, short item, short thickness)
{
	DrawSurface *surface = theDialog->GetWindow()->GetDrawSurface();

	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	const Rect itemRect = itemRef.GetWidget()->GetRect();

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

	const Point bottomLeftCorner = Point::Create(itemRect.left + thickness, itemRect.bottom);
	const Point topRightCorner = Point::Create(itemRect.right, itemRect.top + thickness);
	const Point bottomRightCorner = Point::Create(itemRect.right + thickness, itemRect.bottom + thickness);

	surface->FillRect(Rect::Create(topRightCorner.v, topRightCorner.h, bottomRightCorner.v, bottomRightCorner.h), blackColor);
	surface->FillRect(Rect::Create(bottomLeftCorner.v, bottomLeftCorner.h, bottomRightCorner.v, bottomRightCorner.h), blackColor);
}

//--------------------------------------------------------------  EraseDialogItem
// Erases (but doesn't physically remove) a dialog item.

void EraseDialogItem (Dialog *theDialog, short item)
{
	const PortabilityLayer::DialogItem &itemRef = theDialog->GetItems()[item - 1];
	PortabilityLayer::Widget *widget = itemRef.GetWidget();

	const Rect itemRect = widget->GetRect();

	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();

	DrawSurface *surface = theDialog->GetWindow()->GetDrawSurface();
	surface->FillRect(itemRect, whiteColor);
}

