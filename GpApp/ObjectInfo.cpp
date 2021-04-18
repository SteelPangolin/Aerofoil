
//============================================================================
//----------------------------------------------------------------------------
//								  ObjectInfo.c
//----------------------------------------------------------------------------
//============================================================================

#include "PLKeyEncoding.h"
#include "PLNumberFormatting.h"
#include "PLSound.h"
#include "PLPasStr.h"
#include "DialogManager.h"
#include "DialogUtils.h"
#include "Externs.h"
#include "ObjectEdit.h"
#include "PLStandardColors.h"
#include "PLTimeTaggedVOSEvent.h"
#include "QDPixMap.h"
#include "RectUtils.h"
#include "ResolveCachingColor.h"


#define kBlowerInfoDialogID			1007
#define kFurnitureInfoDialogID		1010
#define kSwitchInfoDialogID			1011
#define kLightInfoDialogID			1013
#define kApplianceInfoDialogID		1014
#define kInvisBonusInfoDialogID		1015
#define kGreaseInfoDialogID			1019
#define kTransInfoDialogID			1022
#define kEnemyInfoDialogID			1027
#define kFlowerInfoDialogID			1033
#define kTriggerInfoDialogID		1034
#define kMicrowaveInfoDialogID		1035
#define kCustPictInfoDialogID		1045
#define kCustPictIDItem				7
#define kInitialStateCheckbox		6
#define kForceCheckbox				7
#define kDirectionText				9
#define kLeftFacingRadio			16
#define kRightFacingRadio			17
#define	kToggleRadio				6
#define	kForceOnRadio				7
#define	kForceOffRadio				8
#define kDelay3Item					6
#define kDelayItem					8
#define kDelayLabelItem				9
#define k100PtRadio					6
#define k300PtRadio					7
#define k500PtRadio					8
#define kGreaseItem					6
#define kLinkTransButton			6
#define kInitialStateCheckbox3		13
#define kTransRoomText				8
#define kTransObjectText			9
#define kKillBandsCheckbox			8
#define kKillBatteryCheckbox		9
#define kKillFoilCheckbox			10
#define kDelay2Item					7
#define kDelay2LabelItem			8
#define kDelay2LabelItem2			9
#define kInitialStateCheckbox2		10
#define kRadioFlower1				6
#define kRadioFlower6				11
#define kFlowerCancel				12
#define kGotoButton1				11
#define kGotoButton2				14


void UpdateBlowerInfo (Dialog *);
void UpdateFurnitureInfo (Dialog *);
void UpdateCustPictInfo (Dialog *);
void UpdateSwitchInfo (Dialog *);
void UpdateTriggerInfo (Dialog *);
void UpdateLightInfo (Dialog *);
void UpdateApplianceInfo (Dialog *);
void UpdateMicrowaveInfo (Dialog *);
void UpdateGreaseInfo (Dialog *);
void UpdateInvisBonusInfo (Dialog *);
void UpdateTransInfo (Dialog *);
void UpdateEnemyInfo (Dialog *);
void UpdateFlowerInfo (Dialog *);
int16_t BlowerFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t FurnitureFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t CustPictFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t SwitchFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t TriggerFilter(Dialog *, const TimeTaggedVOSEvent *evt);
int16_t LightFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t ApplianceFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t MicrowaveFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t GreaseFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t InvisBonusFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t TransFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t EnemyFilter (Dialog *, const TimeTaggedVOSEvent *evt);
int16_t FlowerFilter (Dialog *, const TimeTaggedVOSEvent *evt);
void DoBlowerObjectInfo (short);
void DoFurnitureObjectInfo (void);
void DoCustPictObjectInfo (void);
void DoSwitchObjectInfo (void);
void DoTriggerObjectInfo (void);
void DoLightObjectInfo (void);
void DoApplianceObjectInfo (short);
void DoMicrowaveObjectInfo (void);
void DoGreaseObjectInfo (void);
void DoInvisBonusObjectInfo (void);
void DoTransObjectInfo (short);
void DoEnemyObjectInfo (short);
void DoFlowerObjectInfo (void);


short		newDirection, newPoint;
Byte		newType;

extern	retroLink	retroLinkList[];
extern	short		linkRoom, linkType, wasFlower;
extern	Byte		linkObject;
extern	Boolean		linkerIsSwitch;


#ifndef COMPILEDEMO

//==============================================================  Functions
//--------------------------------------------------------------  UpdateBlowerInfo

void UpdateBlowerInfo (Dialog *theDialog)
{
	#define		kArrowheadLength	4
	Rect		bounds;
	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();

	Window *window = theDialog->GetWindow();
	DrawSurface *surface = window->GetDrawSurface();

	surface->FillRect((*surface->m_port.GetPixMap())->m_rect, whiteColor);

	window->DrawControls();
	
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
	
	if ((thisRoom->objects[objActive].what != kLeftFan) && 
			(thisRoom->objects[objActive].what != kRightFan))
	{
		PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();

		GetDialogItemRect(theDialog, 8, &bounds);
		bounds.right += 2;
		bounds.bottom += 2;

		surface->FillRect(bounds, whiteColor);
		bounds.right -= 2;
		bounds.bottom -= 2;

		for (int16_t offsetChunk = 0; offsetChunk < 4; offsetChunk++)
		{
			PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

			const int16_t xOffset = offsetChunk & 1;
			const int16_t yOffset = (offsetChunk >> 1) & 1;

			const Point offset = Point::Create(xOffset, yOffset);

			switch (newDirection)
			{
				case 1:		// up
				{
					const Point basePoint = offset + Point::Create(bounds.left + HalfRectWide(&bounds), bounds.top);
					surface->DrawLine(basePoint, basePoint + Point::Create(0, RectTall(&bounds)), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(kArrowheadLength, kArrowheadLength), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(-kArrowheadLength, kArrowheadLength), blackColor);
				}
				break;
			
				case 2:		// right
				{
					const Point basePoint = offset + Point::Create(bounds.right, bounds.top + HalfRectTall(&bounds));
					surface->DrawLine(basePoint, basePoint + Point::Create(-RectWide(&bounds), 0), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(-kArrowheadLength, kArrowheadLength), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(-kArrowheadLength, -kArrowheadLength), blackColor);
				}
				break;
			
				case 4:		// down
				{
					const Point basePoint = offset + Point::Create(bounds.left + HalfRectWide(&bounds), bounds.bottom);
					surface->DrawLine(basePoint, basePoint + Point::Create(0, -RectTall(&bounds)), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(kArrowheadLength, -kArrowheadLength), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(-kArrowheadLength, -kArrowheadLength), blackColor);
				}
				break;
			
				case 8:		// left
				{
					const Point basePoint = offset + Point::Create(bounds.left, bounds.top + HalfRectTall(&bounds));
					surface->DrawLine(basePoint, basePoint + Point::Create(RectWide(&bounds), 0), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(kArrowheadLength, -kArrowheadLength), blackColor);
					surface->DrawLine(basePoint, basePoint + Point::Create(kArrowheadLength, kArrowheadLength), blackColor);
				}
				break;
			
				default:
				break;
			}
		}
		
		if ((thisRoom->objects[objActive].what == kInvisBlower) || 
				(thisRoom->objects[objActive].what == kLiftArea))
		{
			PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

			switch (newDirection)
			{
				case 1:		// up
				EraseDialogItem(theDialog, 11);
				FrameOvalDialogItem(theDialog, 12, blackColor);
				FrameOvalDialogItem(theDialog, 13, blackColor);
				FrameOvalDialogItem(theDialog, 14, blackColor);
				break;
				
				case 2:		// right
				FrameOvalDialogItem(theDialog, 11, blackColor);
				EraseDialogItem(theDialog, 12);
				FrameOvalDialogItem(theDialog, 13, blackColor);
				FrameOvalDialogItem(theDialog, 14, blackColor);
				break;
				
				case 4:		// down
				FrameOvalDialogItem(theDialog, 11, blackColor);
				FrameOvalDialogItem(theDialog, 12, blackColor);
				EraseDialogItem(theDialog, 13);
				FrameOvalDialogItem(theDialog, 14, blackColor);
				break;
				
				case 8:		// left
				FrameOvalDialogItem(theDialog, 11, blackColor);
				FrameOvalDialogItem(theDialog, 12, blackColor);
				FrameOvalDialogItem(theDialog, 13, blackColor);
				EraseDialogItem(theDialog, 14);
				break;
			}
		}
	}
}

//--------------------------------------------------------------  UpdateFurnitureInfo

void UpdateFurnitureInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateCustPictInfo

void UpdateCustPictInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateSwitchInfo

void UpdateSwitchInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	SelectFromRadioGroup(theDialog, newType + kToggleRadio, 
			kToggleRadio, kForceOffRadio);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
	FrameDialogItemC(theDialog, 13, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateTriggerInfo

void UpdateTriggerInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
	FrameDialogItemC(theDialog, 13, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateLightInfo

void UpdateLightInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateApplianceInfo

void UpdateApplianceInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateMicrowaveInfo

void UpdateMicrowaveInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateGreaseInfo

void UpdateGreaseInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 5, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateInvisBonusInfo

void UpdateInvisBonusInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	SelectFromRadioGroup(theDialog, newPoint + k100PtRadio, 
			k100PtRadio, k500PtRadio);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateTransInfo

void UpdateTransInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
	FrameDialogItemC(theDialog, 10, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateEnemyInfo

void UpdateEnemyInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
}

//--------------------------------------------------------------  UpdateFlowerInfo

void UpdateFlowerInfo (Dialog *theDialog)
{
	DrawDefaultButton(theDialog);
	FrameDialogItemC(theDialog, 4, kRedOrangeColor8);
}

//--------------------------------------------------------------  BlowerFilter

int16_t BlowerFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		const GpKeyboardInputEvent &keyboardEvent = evt->m_vosEvent.m_event.m_keyboardInputEvent;

		switch (PackVOSKeyCode(keyboardEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		case PL_KEY_SPECIAL(kTab):
			// SelectDialogItemText(dial, kRoomNameItem, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  FurnitureFilter

int16_t FurnitureFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		const GpKeyboardInputEvent &keyEvt = evt->m_vosEvent.m_event.m_keyboardInputEvent;

		switch (PackVOSKeyCode(keyEvt))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  CustPictFilter

int16_t CustPictFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  SwitchFilter

int16_t SwitchFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  TriggerFilter

int16_t TriggerFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		case PL_KEY_SPECIAL(kTab):
			SelectDialogItemText(dial, kDelay3Item, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  LightFilter

int16_t LightFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  ApplianceFilter

int16_t ApplianceFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		case PL_KEY_SPECIAL(kTab):
			SelectDialogItemText(dial, kDelayItem, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  MicrowaveFilter

int16_t MicrowaveFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kTab):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  GreaseFilter

int16_t GreaseFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  InvisBonusFilter

int16_t InvisBonusFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  TransFilter

int16_t TransFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  EnemyFilter

int16_t EnemyFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		case PL_KEY_SPECIAL(kTab):
			SelectDialogItemText(dial, kDelay2Item, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  FlowerFilter

int16_t FlowerFilter (void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		default:
			return -1;
		}
	}

	return -1;
}

//--------------------------------------------------------------  DoBlowerObjectInfo

void DoBlowerObjectInfo (short what)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr, distStr;
	short			item, initial;
	Boolean			leaving, doReturn, leftFacing;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	NumToString(thisRoom->objects[objActive].data.a.distance, distStr);

	DialogTextSubstitutions substitutions(numberStr, kindStr, distStr);
	
//	CenterDialog(kBlowerInfoDialogID);
	infoDial = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(kBlowerInfoDialogID, kPutInFront, &substitutions);
	if (infoDial == nil)
		RedAlert(kErrDialogDidntLoad);
	
	newDirection = thisRoom->objects[objActive].data.a.vector & 0x0F;
	if (thisRoom->objects[objActive].data.a.initial)
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 0);
	
	if ((what == kTaper) || (what == kCandle) || (what == kStubby) || 
			(what == kTiki) || (what == kBBQ))
	{
		HideDialogItem(infoDial, kInitialStateCheckbox);
	}
	
	if ((what == kLeftFan) || (what == kRightFan))
	{
		if (what == kLeftFan)
		{
			SelectFromRadioGroup(infoDial, kLeftFacingRadio, 
					kLeftFacingRadio, kRightFacingRadio);
			leftFacing = true;
		}
		else
		{
			SelectFromRadioGroup(infoDial, kRightFacingRadio, 
					kLeftFacingRadio, kRightFacingRadio);
			leftFacing = false;
		}
		HideDialogItem(infoDial, kDirectionText);
	}
	else
	{
		HideDialogItem(infoDial, kLeftFacingRadio);
		HideDialogItem(infoDial, kRightFacingRadio);
	}
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 15);

	UpdateBlowerInfo(infoDial);

	ShowWindow(infoDial->GetWindow());
	
	leaving = false;
	doReturn = false;
	
	while (!leaving)
	{
		bool redrawMain = false;

		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(BlowerFilter));
		
		if (item == kOkayButton)
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.a.initial = true;
			else
				thisRoom->objects[objActive].data.a.initial = false;
			thisRoom->objects[objActive].data.a.vector = (Byte)newDirection;
			if ((what == kLeftFan) || (what == kRightFan))
			{
				if (leftFacing)
					thisRoom->objects[objActive].what = kLeftFan;
				else
					thisRoom->objects[objActive].what = kRightFan;
				if (KeepObjectLegal())
				{
				}
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
				UpdateMainWindow();
			}
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kInitialStateCheckbox)
			ToggleDialogItemValue(infoDial, kInitialStateCheckbox);
		else if (item == 15)			// Linked From? button.
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.a.initial = true;
			else
				thisRoom->objects[objActive].data.a.initial = false;
			thisRoom->objects[objActive].data.a.vector = (Byte)newDirection;
			if ((what == kLeftFan) || (what == kRightFan))
			{
				if (leftFacing)
					thisRoom->objects[objActive].what = kLeftFan;
				else
					thisRoom->objects[objActive].what = kRightFan;
				if (KeepObjectLegal())
				{
				}
				redrawMain = true;
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
			}
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doReturn = true;
		}
		else if (item == kLeftFacingRadio)
		{
			leftFacing = true;
			SelectFromRadioGroup(infoDial, kLeftFacingRadio, kLeftFacingRadio, 
					kRightFacingRadio);
		}
		else if (item == kRightFacingRadio)
		{
			leftFacing = false;
			SelectFromRadioGroup(infoDial, kRightFacingRadio, kLeftFacingRadio, 
					kRightFacingRadio);
		}
		else if ((thisRoom->objects[objActive].what == kInvisBlower) || 
				(thisRoom->objects[objActive].what == kLiftArea))
		{
			switch (item)
			{
				case 11:
				newDirection = 0x01;
				break;
				
				case 12:
				newDirection = 0x02;
				break;
				
				case 13:
				newDirection = 0x04;
				break;
				
				case 14:
				newDirection = 0x08;
				break;
			}
			UpdateBlowerInfo(infoDial);
		}

		if (redrawMain)
			UpdateMainWindow();
	}
	
	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoFurnitureObjectInfo

void DoFurnitureObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item;
	Boolean			leaving, doReturn;
	
	if (objActive == kInitialGliderSelected)
	{
		PasStringCopy(PSTR("-"), numberStr);
		PasStringCopy(PSTR("Glider Begins"), kindStr);
	}
	else if (objActive == kLeftGliderSelected)
	{
		PasStringCopy(PSTR("-"), numberStr);
		PasStringCopy(PSTR("New Glider (left)"), kindStr);
	}
	else if (objActive == kRightGliderSelected)
	{
		PasStringCopy(PSTR("-"), numberStr);
		PasStringCopy(PSTR("New Glider (right)"), kindStr);
	}
	else
	{
		NumToString(objActive + 1, numberStr);
		GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	}

	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kFurnitureInfoDialogID, &substitutions);
	
	if ((objActive < 0) || (retroLinkList[objActive].room == -1))
		HideDialogItem(infoDial, 6);

	UpdateFurnitureInfo(infoDial);
	
	leaving = false;
	doReturn = false;
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(FurnitureFilter));
		
		if (item == kOkayButton)
			leaving = true;
		else if (item == 6)				// Linked From? button.
		{
			leaving = true;
			doReturn = true;
		}
	}
	
	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoCustPictObjectInfo

void DoCustPictObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	long			wasPict;
	short			item;
	Boolean			leaving;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);

	DialogTextSubstitutions substitutions;

	if (thisRoom->objects[objActive].what == kCustomPict)
		substitutions = DialogTextSubstitutions(numberStr, kindStr, PSTR("PICT"), PSTR("10000"));
	else
		substitutions = DialogTextSubstitutions(numberStr, kindStr, PSTR("Sound"), PSTR("3000"));
	
	BringUpDialog(&infoDial, kCustPictInfoDialogID, &substitutions);
	if (thisRoom->objects[objActive].what == kCustomPict)
	{
		wasPict = (long)(thisRoom->objects[objActive].data.g.height);
		SetDialogNumToStr(infoDial, kCustPictIDItem, wasPict);
	}
	else
	{
		wasPict = (long)(thisRoom->objects[objActive].data.e.where);
		SetDialogNumToStr(infoDial, kCustPictIDItem, wasPict);
	}
	SelectDialogItemText(infoDial, kCustPictIDItem, 0, 1024);
	leaving = false;

	UpdateCustPictInfo(infoDial);

	while (!leaving)
	{
		bool redrawMain = false;

		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(CustPictFilter));
		
		if (item == kOkayButton)
		{
			GetDialogNumFromStr(infoDial, kCustPictIDItem, &wasPict);
			if (thisRoom->objects[objActive].what == kCustomPict)
			{
				if ((wasPict < 10000L) || (wasPict > 32767L))
				{
					SysBeep(1);
					wasPict = (long)(thisRoom->objects[objActive].data.g.height);
					SetDialogNumToStr(infoDial, kCustPictIDItem, wasPict);
					SelectDialogItemText(infoDial, kCustPictIDItem, 0, 1024);
				}
				else
				{
					thisRoom->objects[objActive].data.g.height = (short)wasPict;
					if (KeepObjectLegal())
					{
					}
					fileDirty = true;
					UpdateMenus(false);
					redrawMain = true;
					GetThisRoomsObjRects();
					ReadyBackground(thisRoom->background, thisRoom->tiles);
					DrawThisRoomsObjects();
					leaving = true;
				}
			}
			else
			{
				if ((wasPict < 3000L) || (wasPict > 32767L))
				{
					SysBeep(1);
					wasPict = (long)(thisRoom->objects[objActive].data.e.where);
					SetDialogNumToStr(infoDial, kCustPictIDItem, wasPict);
					SelectDialogItemText(infoDial, kCustPictIDItem, 0, 1024);
				}
				else
				{
					thisRoom->objects[objActive].data.e.where = (short)wasPict;
					fileDirty = true;
					UpdateMenus(false);
					redrawMain = true;
					GetThisRoomsObjRects();
					ReadyBackground(thisRoom->background, thisRoom->tiles);
					DrawThisRoomsObjects();
					leaving = true;
				}
			}
		}
		else if (item == kCancelButton)
		{
			leaving = true;
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
}

//--------------------------------------------------------------  DoSwitchObjectInfo

void DoSwitchObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr, roomStr, tempStr, objStr;
	short			item, floor, suite;
	Boolean			leaving, doLink, doGoTo, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	if (thisRoom->objects[objActive].data.e.where == -1)
		PasStringCopy(PSTR("none"), roomStr);
	else
	{
		ExtractFloorSuite(thisRoom->objects[objActive].data.e.where, &floor, &suite);
		NumToString((long)floor, roomStr);
		PasStringConcat(roomStr, PSTR(" / "));
		NumToString((long)suite, tempStr);
		PasStringConcat(roomStr, tempStr);
	}
	
	if (thisRoom->objects[objActive].data.e.who == 255)
		PasStringCopy(PSTR("none"), objStr);
	else
		NumToString((long)thisRoom->objects[objActive].data.e.who + 1, objStr);

	newType = thisRoom->objects[objActive].data.e.type;

	DialogTextSubstitutions substitutions(numberStr, kindStr, roomStr, objStr);
	BringUpDialog(&infoDial, kSwitchInfoDialogID, &substitutions);
	leaving = false;
	doLink = false;
	doGoTo = false;
	doReturn = false;
	
	if (thisRoom->objects[objActive].data.e.who == 255)
		MyDisableControl(infoDial, kGotoButton2);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 15);

	UpdateSwitchInfo(infoDial);
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(SwitchFilter));
		
		if (item == kOkayButton)
		{
			thisRoom->objects[objActive].data.e.type = newType;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kToggleRadio)
		{
			SelectFromRadioGroup(infoDial, item, kToggleRadio, kForceOffRadio);
			newType = kToggle;
		}
		else if (item == kForceOnRadio)
		{
			SelectFromRadioGroup(infoDial, item, kToggleRadio, kForceOffRadio);
			newType = kForceOn;
		}
		else if (item == kForceOffRadio)
		{
			SelectFromRadioGroup(infoDial, item, kToggleRadio, kForceOffRadio);
			newType = kForceOff;
		}
		else if (item == 9)
		{
			thisRoom->objects[objActive].data.e.type = newType;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doLink = true;
		}
		else if (item == kGotoButton2)
		{
			thisRoom->objects[objActive].data.e.type = newType;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doGoTo = true;
		}
		else if (item == 15)			// Linked From? button.
		{
			thisRoom->objects[objActive].data.e.type = newType;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doReturn = true;
		}
	}

	infoDial->Destroy();
	
	if (doLink)
	{
		linkType = kSwitchLinkOnly;
		linkerIsSwitch = true;
		OpenLinkWindow();
		linkRoom = thisRoomNumber;
		linkObject = (Byte)objActive;
		DeselectObject();
	}
	else if (doGoTo)
	{
		GoToObjectInRoom((short)thisRoom->objects[objActive].data.e.who, floor, suite);
	}
	else if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoTriggerObjectInfo

void DoTriggerObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr, roomStr, tempStr, objStr;
	long			delayIs;
	short			item, floor, suite;
	Boolean			leaving, doLink, doGoTo, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	if (thisRoom->objects[objActive].data.e.where == -1)
		PasStringCopy(PSTR("none"), roomStr);
	else
	{
		ExtractFloorSuite(thisRoom->objects[objActive].data.e.where, &floor, &suite);
		NumToString((long)floor, roomStr);
		PasStringConcat(roomStr, PSTR(" / "));
		NumToString((long)suite, tempStr);
		PasStringConcat(roomStr, tempStr);
	}
	
	if (thisRoom->objects[objActive].data.e.who == 255)
		PasStringCopy(PSTR("none"), objStr);
	else
		NumToString((long)thisRoom->objects[objActive].data.e.who + 1, objStr);

	DialogTextSubstitutions substitutions(numberStr, kindStr, roomStr, objStr);
	newType = thisRoom->objects[objActive].data.e.type;
	
	BringUpDialog(&infoDial, kTriggerInfoDialogID, &substitutions);
	leaving = false;
	doLink = false;
	doGoTo = false;
	doReturn = false;
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 15);
	
	if (thisRoom->objects[objActive].data.e.who == 255)
		MyDisableControl(infoDial, kGotoButton2);
	
	SetDialogNumToStr(infoDial, kDelay3Item, 
			(long)thisRoom->objects[objActive].data.e.delay);
	SelectDialogItemText(infoDial, kDelay3Item, 0, 1024);

	UpdateTriggerInfo(infoDial);
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(TriggerFilter));
		
		if (item == kOkayButton)
		{
			GetDialogNumFromStr(infoDial, kDelay3Item, &delayIs);
			if ((delayIs < 0L) || (delayIs > 32767L))
			{
				SysBeep(1);
				SetDialogNumToStr(infoDial, kDelay3Item, 
						(long)thisRoom->objects[objActive].data.e.delay);
				SelectDialogItemText(infoDial, kDelay3Item, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.e.delay = (short)delayIs;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
			}
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == 9)
		{
			GetDialogNumFromStr(infoDial, kDelay3Item, &delayIs);
			if ((delayIs < 0L) || (delayIs > 32767L))
			{
				SysBeep(1);
				SetDialogNumToStr(infoDial, kDelay3Item, 
						(long)thisRoom->objects[objActive].data.e.delay);
				SelectDialogItemText(infoDial, kDelay3Item, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.e.delay = (short)delayIs;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
				doLink = true;
			}
		}
		else if (item == kGotoButton2)
		{
			GetDialogNumFromStr(infoDial, kDelay3Item, &delayIs);
			if ((delayIs < 0L) || (delayIs > 32767L))
			{
				SysBeep(1);
				SetDialogNumToStr(infoDial, kDelay3Item, 
						(long)thisRoom->objects[objActive].data.e.delay);
				SelectDialogItemText(infoDial, kDelay3Item, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.e.delay = (short)delayIs;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
				doGoTo = true;
			}
		}
		else if (item == 15)			// Linked From? button.
		{
			GetDialogNumFromStr(infoDial, kDelay3Item, &delayIs);
			if ((delayIs < 0L) || (delayIs > 32767L))
			{
				SysBeep(1);
				SetDialogNumToStr(infoDial, kDelay3Item, 
						(long)thisRoom->objects[objActive].data.e.delay);
				SelectDialogItemText(infoDial, kDelay3Item, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.e.delay = (short)delayIs;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
				doReturn = true;
			}
		}
	}

	infoDial->Destroy();
	
	if (doLink)
	{
		linkType = kTriggerLinkOnly;
		linkerIsSwitch = true;
		OpenLinkWindow();
		linkRoom = thisRoomNumber;
		linkObject = (Byte)objActive;
		DeselectObject();
	}
	else if (doGoTo)
	{
		GoToObjectInRoom((short)thisRoom->objects[objActive].data.e.who, floor, suite);
	}
	else if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoLightObjectInfo

void DoLightObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item, initial;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);

	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
//	CenterDialog(kLightInfoDialogID);
	infoDial = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(kLightInfoDialogID, kPutInFront, &substitutions);
	if (infoDial == nil)
		RedAlert(kErrDialogDidntLoad);
	
	if (thisRoom->objects[objActive].data.f.initial)
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 0);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 8);
	
	ShowWindow(infoDial->GetWindow());
	
	leaving = false;
	doReturn = false;

	UpdateLightInfo(infoDial);
	
	while (!leaving)
	{
		bool redrawMain = false;
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(LightFilter));
		
		if (item == kOkayButton)
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.f.initial = true;
			else
				thisRoom->objects[objActive].data.f.initial = false;
			
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			redrawMain = true;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kInitialStateCheckbox)
			ToggleDialogItemValue(infoDial, kInitialStateCheckbox);
		else if (item == 8)			// Linked From? button.
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.f.initial = true;
			else
				thisRoom->objects[objActive].data.f.initial = false;
			
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			redrawMain = true;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doReturn = true;
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoApplianceObjectInfo

void DoApplianceObjectInfo (short what)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	long			delay;
	short			item, initial;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kApplianceInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 10);
	
	if (thisRoom->objects[objActive].data.g.initial)
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 0);
	
	if ((what == kShredder) || (what == kMacPlus) || (what == kTV) || 
			(what == kCoffee) || (what == kVCR) || (what == kMicrowave))
	{
		HideDialogItem(infoDial, kDelayItem);
		HideDialogItem(infoDial, kDelayLabelItem);
	}
	
	delay = thisRoom->objects[objActive].data.g.delay;
	SetDialogNumToStr(infoDial, kDelayItem, (long)delay);
	SelectDialogItemText(infoDial, kDelayItem, 0, 1024);
	
	leaving = false;
	doReturn = false;

	UpdateApplianceInfo(infoDial);
	
	while (!leaving)
	{
		bool redrawMain = false;
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(ApplianceFilter));
		
		if (item == kOkayButton)
		{
			GetDialogNumFromStr(infoDial, kDelayItem, &delay);
			if ((delay < 0L) || (delay > 255L))
			{
				SysBeep(0);
				delay = thisRoom->objects[objActive].data.g.delay;
				SetDialogNumToStr(infoDial, kDelayItem, (long)delay);
				SelectDialogItemText(infoDial, kDelayItem, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.g.delay = (Byte)delay;
				GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
				if (initial == 1)
					thisRoom->objects[objActive].data.g.initial = true;
				else
					thisRoom->objects[objActive].data.g.initial = false;
				fileDirty = true;
				UpdateMenus(false);
				redrawMain = true;
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
				leaving = true;
			}
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kInitialStateCheckbox)
			ToggleDialogItemValue(infoDial, kInitialStateCheckbox);
		else if (item == 10)			// Linked From? button.
		{
			GetDialogNumFromStr(infoDial, kDelayItem, &delay);
			if ((delay < 0L) || (delay > 255L))
			{
				SysBeep(0);
				delay = thisRoom->objects[objActive].data.g.delay;
				SetDialogNumToStr(infoDial, kDelayItem, (long)delay);
				SelectDialogItemText(infoDial, kDelayItem, 0, 1024);
			}
			else
			{
				thisRoom->objects[objActive].data.g.delay = (Byte)delay;
				GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
				if (initial == 1)
					thisRoom->objects[objActive].data.g.initial = true;
				else
					thisRoom->objects[objActive].data.g.initial = false;
				fileDirty = true;
				UpdateMenus(false);
				redrawMain = true;
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
				leaving = true;
				doReturn = true;
			}
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoMicrowaveObjectInfo

void DoMicrowaveObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item, initial, kills;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kMicrowaveInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 11);
	
	if (thisRoom->objects[objActive].data.g.initial)
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kInitialStateCheckbox, 0);
	
	kills = (short)thisRoom->objects[objActive].data.g.byte0;
	if ((kills & 0x0001) == 0x0001)
		SetDialogItemValue(infoDial, kKillBandsCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kKillBandsCheckbox, 0);
	if ((kills & 0x0002) == 0x0002)
		SetDialogItemValue(infoDial, kKillBatteryCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kKillBatteryCheckbox, 0);
	if ((kills & 0x0004) == 0x0004)
		SetDialogItemValue(infoDial, kKillFoilCheckbox, 1);
	else
		SetDialogItemValue(infoDial, kKillFoilCheckbox, 0);
	
	leaving = false;
	doReturn = false;

	UpdateMicrowaveInfo(infoDial);
	
	while (!leaving)
	{
		bool redrawMain = false;
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(MicrowaveFilter));
		
		if (item == kOkayButton)
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.g.initial = true;
			else
				thisRoom->objects[objActive].data.g.initial = false;
			kills = 0;
			GetDialogItemValue(infoDial, kKillBandsCheckbox, &initial);
			if (initial == 1)
				kills += 1;
			GetDialogItemValue(infoDial, kKillBatteryCheckbox, &initial);
			if (initial == 1)
				kills += 2;
			GetDialogItemValue(infoDial, kKillFoilCheckbox, &initial);
			if (initial == 1)
				kills += 4;
			thisRoom->objects[objActive].data.g.byte0 = (Byte)kills;
			
			fileDirty = true;
			UpdateMenus(false);
			redrawMain = true;
			GetThisRoomsObjRects();
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kInitialStateCheckbox)
			ToggleDialogItemValue(infoDial, kInitialStateCheckbox);
		else if (item == kKillBandsCheckbox)
			ToggleDialogItemValue(infoDial, kKillBandsCheckbox);
		else if (item == kKillBatteryCheckbox)
			ToggleDialogItemValue(infoDial, kKillBatteryCheckbox);
		else if (item == kKillFoilCheckbox)
			ToggleDialogItemValue(infoDial, kKillFoilCheckbox);
		else if (item == 11)			// Linked From? button.
		{
			GetDialogItemValue(infoDial, kInitialStateCheckbox, &initial);
			if (initial == 1)
				thisRoom->objects[objActive].data.g.initial = true;
			else
				thisRoom->objects[objActive].data.g.initial = false;
			kills = 0;
			GetDialogItemValue(infoDial, kKillBandsCheckbox, &initial);
			if (initial == 1)
				kills += 1;
			GetDialogItemValue(infoDial, kKillBatteryCheckbox, &initial);
			if (initial == 1)
				kills += 2;
			GetDialogItemValue(infoDial, kKillFoilCheckbox, &initial);
			if (initial == 1)
				kills += 4;
			thisRoom->objects[objActive].data.g.byte0 = (Byte)kills;
			
			fileDirty = true;
			UpdateMenus(false);
			redrawMain = true;
			GetThisRoomsObjRects();
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			leaving = true;
			doReturn = true;
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoGreaseObjectInfo

void DoGreaseObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item;
	Boolean			leaving, wasSpilled, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kGreaseInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 8);
	
	wasSpilled = !(thisRoom->objects[objActive].data.c.initial);
	SetDialogItemValue(infoDial, kGreaseItem, (short)wasSpilled);
	leaving = false;
	doReturn = false;

	UpdateGreaseInfo(infoDial);
	
	while (!leaving)
	{
		bool redrawMain = false;

		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(GreaseFilter));
		
		if (item == kOkayButton)
		{
			thisRoom->objects[objActive].data.c.initial = !wasSpilled;
			fileDirty = true;
			UpdateMenus(false);
			redrawMain = true;
			GetThisRoomsObjRects();
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			leaving = true;
		}
		else if (item == kCancelButton)
		{
			leaving = true;
		}
		else if (item == kGreaseItem)
		{
			wasSpilled = !wasSpilled;
			SetDialogItemValue(infoDial, kGreaseItem, (short)wasSpilled);
		}
		else if (item == 8)				// Linked From? button.
		{
			thisRoom->objects[objActive].data.c.initial = !wasSpilled;
			fileDirty = true;
			UpdateMenus(false);
			redrawMain = true;
			GetThisRoomsObjRects();
			ReadyBackground(thisRoom->background, thisRoom->tiles);
			DrawThisRoomsObjects();
			leaving = true;
			doReturn = true;
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoInvisBonusObjectInfo

void DoInvisBonusObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	switch (thisRoom->objects[objActive].data.c.points)
	{
		case 300:
		newPoint = 1;
		break;
		
		case 500:
		newPoint = 2;
		break;
		
		default:
		newPoint = 0;
		break;
	}
	
	BringUpDialog(&infoDial, kInvisBonusInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 9);
	
	leaving = false;
	doReturn = false;

	UpdateInvisBonusInfo(infoDial);
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(InvisBonusFilter));
		
		if (item == kOkayButton)
		{
			switch (newPoint)
			{
				case 0:
				thisRoom->objects[objActive].data.c.points = 100;
				break;
				
				case 1:
				thisRoom->objects[objActive].data.c.points = 300;
				break;
				
				case 2:
				thisRoom->objects[objActive].data.c.points = 500;
				break;
			}
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == k100PtRadio)
		{
			SelectFromRadioGroup(infoDial, item, k100PtRadio, k500PtRadio);
			newPoint = 0;
		}
		else if (item == k300PtRadio)
		{
			SelectFromRadioGroup(infoDial, item, k100PtRadio, k500PtRadio);
			newPoint = 1;
		}
		else if (item == k500PtRadio)
		{
			SelectFromRadioGroup(infoDial, item, k100PtRadio, k500PtRadio);
			newPoint = 2;
		}
		else if (item == 9)				// Linked From? button.
		{
			switch (newPoint)
			{
				case 0:
				thisRoom->objects[objActive].data.c.points = 100;
				break;
				
				case 1:
				thisRoom->objects[objActive].data.c.points = 300;
				break;
				
				case 2:
				thisRoom->objects[objActive].data.c.points = 500;
				break;
			}
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doReturn = true;
		}
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoTransObjectInfo

void DoTransObjectInfo (short what)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr, roomStr, tempStr, objStr;
	short			item, floor, suite;
	Boolean			leaving, doLink, doGoTo, doReturn, wasState;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	if (thisRoom->objects[objActive].data.d.where == -1)
		PasStringCopy(PSTR("none"), roomStr);
	else
	{
		ExtractFloorSuite(thisRoom->objects[objActive].data.d.where, &floor, &suite);
		NumToString((long)floor, roomStr);
		PasStringConcat(roomStr, PSTR(" / "));
		NumToString((long)suite, tempStr);
		PasStringConcat(roomStr, tempStr);
	}
	
	if (thisRoom->objects[objActive].data.d.who == 255)
		PasStringCopy(PSTR("none"), objStr);
	else
		NumToString((long)thisRoom->objects[objActive].data.d.who + 1, objStr);

	DialogTextSubstitutions substitutions(numberStr, kindStr, roomStr, objStr);
	
	BringUpDialog(&infoDial, kTransInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 12);
	if (what != kDeluxeTrans)
		HideDialogItem(infoDial, kInitialStateCheckbox3);
	else
	{
		wasState = (thisRoom->objects[objActive].data.d.wide & 0xF0) >> 4;
		SetDialogItemValue(infoDial, kInitialStateCheckbox3, (short)wasState);
	}
	
	leaving = false;
	doLink = false;
	doGoTo = false;
	doReturn = false;
	
	if (thisRoom->objects[objActive].data.d.who == 255)
		MyDisableControl(infoDial, kGotoButton1);

	UpdateTransInfo(infoDial);
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(TransFilter));
		
		if (item == kOkayButton)
		{
			if (what == kDeluxeTrans)
				thisRoom->objects[objActive].data.d.wide = wasState << 4;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kLinkTransButton)
		{
			if (what == kDeluxeTrans)
				thisRoom->objects[objActive].data.d.wide = wasState << 4;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doLink = true;
		}
		else if (item == kGotoButton1)
		{
			if (what == kDeluxeTrans)
				thisRoom->objects[objActive].data.d.wide = wasState << 4;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doGoTo = true;
		}
		else if (item == 12)			// Linked From? button.
		{
			if (what == kDeluxeTrans)
				thisRoom->objects[objActive].data.d.wide = wasState << 4;
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
			doReturn = true;
		}
		else if (item == kInitialStateCheckbox3)
		{
			wasState = !wasState;
			SetDialogItemValue(infoDial, kInitialStateCheckbox3, (short)wasState);
		}
	}

	infoDial->Destroy();
	
	if (doLink)
	{
		linkType = kTransportLinkOnly;
		linkerIsSwitch = false;
		OpenLinkWindow();
		linkRoom = thisRoomNumber;
		linkObject = (Byte)objActive;
		DeselectObject();
	}
	else if (doGoTo)
	{
		GoToObjectInRoom((short)thisRoom->objects[objActive].data.d.who, floor, suite);
	}
	else if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoEnemyObjectInfo

void DoEnemyObjectInfo (short what)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	long			delay;
	short			item, initial;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kEnemyInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 11);
	
	delay = thisRoom->objects[objActive].data.h.delay;
	SetDialogNumToStr(infoDial, kDelay2Item, (long)delay);
	SelectDialogItemText(infoDial, kDelay2Item, 0, 1024);
	
	if (thisRoom->objects[objActive].data.h.initial)
		SetDialogItemValue(infoDial, kInitialStateCheckbox2, 1);
	else
		SetDialogItemValue(infoDial, kInitialStateCheckbox2, 0);
	
	if (what == kBall)
	{
		HideDialogItem(infoDial, kDelay2Item);
		HideDialogItem(infoDial, 8);
		HideDialogItem(infoDial, 9);
	}
	
	leaving = false;
	doReturn = false;

	UpdateEnemyInfo(infoDial);
	
	while (!leaving)
	{
		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(EnemyFilter));
		
		if (item == kOkayButton)
		{
			GetDialogNumFromStr(infoDial, kDelay2Item, &delay);
			if (((delay < 0L) || (delay > 255L)) && (what != kBall))
			{
				SysBeep(0);
				delay = thisRoom->objects[objActive].data.h.delay;
				SetDialogNumToStr(infoDial, kDelay2Item, (long)delay);
				SelectDialogItemText(infoDial, kDelay2Item, 0, 1024);
			}
			else
			{
				GetDialogItemValue(infoDial, kInitialStateCheckbox2, &initial);
				if (initial == 1)
					thisRoom->objects[objActive].data.h.initial = true;
				else
					thisRoom->objects[objActive].data.h.initial = false;
				if (what != kBall)
					thisRoom->objects[objActive].data.h.delay = (Byte)delay;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
			}
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kInitialStateCheckbox2)
			ToggleDialogItemValue(infoDial, kInitialStateCheckbox2);
		else if (item == 11)			// Linked From? button.
		{
			GetDialogNumFromStr(infoDial, kDelay2Item, &delay);
			if (((delay < 0L) || (delay > 255L)) && (what != kBall))
			{
				SysBeep(0);
				delay = thisRoom->objects[objActive].data.h.delay;
				SetDialogNumToStr(infoDial, kDelay2Item, (long)delay);
				SelectDialogItemText(infoDial, kDelay2Item, 0, 1024);
			}
			else
			{
				GetDialogItemValue(infoDial, kInitialStateCheckbox2, &initial);
				if (initial == 1)
					thisRoom->objects[objActive].data.h.initial = true;
				else
					thisRoom->objects[objActive].data.h.initial = false;
				if (what != kBall)
					thisRoom->objects[objActive].data.h.delay = (Byte)delay;
				fileDirty = true;
				UpdateMenus(false);
				leaving = true;
				doReturn = true;
			}
		}
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoFlowerObjectInfo

void DoFlowerObjectInfo (void)
{
	Dialog			*infoDial;
	Str255			numberStr, kindStr;
	short			item, flower;
	Boolean			leaving, doReturn;
	
	NumToString(objActive + 1, numberStr);
	GetIndString(kindStr, kObjectNameStrings, thisRoom->objects[objActive].what);
	DialogTextSubstitutions substitutions(numberStr, kindStr);
	
	BringUpDialog(&infoDial, kFlowerInfoDialogID, &substitutions);
	
	if (retroLinkList[objActive].room == -1)
		HideDialogItem(infoDial, 13);
	
	flower = thisRoom->objects[objActive].data.i.pict + kRadioFlower1;
	SelectFromRadioGroup(infoDial, flower, kRadioFlower1, kRadioFlower6);
	
	leaving = false;
	doReturn = false;

	UpdateFlowerInfo(infoDial);
	
	while (!leaving)
	{
		bool redrawMain = false;

		item = infoDial->ExecuteModal(nullptr, PL_FILTER_FUNC(FlowerFilter));
		
		if (item == kOkayButton)
		{
			flower -= kRadioFlower1;
			if (flower != thisRoom->objects[objActive].data.i.pict)
			{
				thisRoom->objects[objActive].data.i.bounds.right = 
						thisRoom->objects[objActive].data.i.bounds.left + 
						RectWide(&flowerSrc[flower]);
				thisRoom->objects[objActive].data.i.bounds.top = 
						thisRoom->objects[objActive].data.i.bounds.bottom - 
						RectTall(&flowerSrc[flower]);
				thisRoom->objects[objActive].data.i.pict = flower;
				redrawMain = true;
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
				fileDirty = true;
				UpdateMenus(false);
				wasFlower = flower;
			}
			leaving = true;
		}
		else if ((item >= kRadioFlower1) && (item <= kRadioFlower6))
		{
			flower = item;
			SelectFromRadioGroup(infoDial, flower, kRadioFlower1, kRadioFlower6);
		}
		else if (item == kFlowerCancel)
		{
			leaving = true;
		}
		else if (item == 13)			// Linked From? button.
		{
			flower -= kRadioFlower1;
			if (flower != thisRoom->objects[objActive].data.i.pict)
			{
				thisRoom->objects[objActive].data.i.bounds.right = 
						thisRoom->objects[objActive].data.i.bounds.left + 
						RectWide(&flowerSrc[flower]);
				thisRoom->objects[objActive].data.i.bounds.top = 
						thisRoom->objects[objActive].data.i.bounds.bottom - 
						RectTall(&flowerSrc[flower]);
				thisRoom->objects[objActive].data.i.pict = flower;
				redrawMain = true;
				GetThisRoomsObjRects();
				ReadyBackground(thisRoom->background, thisRoom->tiles);
				DrawThisRoomsObjects();
				fileDirty = true;
				UpdateMenus(false);
				wasFlower = flower;
			}
			leaving = true;
			doReturn = true;
		}

		if (redrawMain)
			UpdateMainWindow();
	}

	infoDial->Destroy();
	
	if (doReturn)
	{
		GoToObjectInRoomNum(retroLinkList[objActive].object, 
				retroLinkList[objActive].room);
	}
}

//--------------------------------------------------------------  DoObjectInfo

void DoObjectInfo (void)
{
	if ((objActive == kInitialGliderSelected) || 
			(objActive == kLeftGliderSelected) || 
			(objActive == kRightGliderSelected))
	{
		DoFurnitureObjectInfo();
		return;
	}
	
	switch (thisRoom->objects[objActive].what)
	{
		case kFloorVent:
		case kCeilingVent:
		case kFloorBlower:
		case kCeilingBlower:
		case kSewerGrate:
		case kLeftFan:
		case kRightFan:
		case kTaper:
		case kCandle:
		case kStubby:
		case kTiki:
		case kBBQ:
		case kInvisBlower:
		case kGrecoVent:
		case kSewerBlower:
		case kLiftArea:
		DoBlowerObjectInfo(thisRoom->objects[objActive].what);
		break;
		
		case kTable:
		case kShelf:
		case kCabinet:
		case kFilingCabinet:
		case kWasteBasket:
		case kMilkCrate:
		case kCounter:
		case kDresser:
		case kDeckTable:
		case kStool:
		case kTrunk:
		case kInvisObstacle:
		case kManhole:
		case kBooks:
		case kInvisBounce:
		case kRedClock:
		case kBlueClock:
		case kYellowClock:
		case kCuckoo:
		case kPaper:
		case kBattery:
		case kBands:
		case kFoil:
		case kStar:
		case kSparkle:
		case kHelium:
		case kSlider:
		case kUpStairs:
		case kDownStairs:
		case kDoorInLf:
		case kDoorInRt:
		case kDoorExRt:
		case kDoorExLf:
		case kWindowInLf:
		case kWindowInRt:
		case kWindowExRt:
		case kWindowExLf:
		case kCinderBlock:
		case kFlowerBox:
		case kCDs:
		case kGuitar:
		case kStereo:
		case kCobweb:
		case kOzma:
		case kMirror:
		case kMousehole:
		case kFireplace:
		case kWallWindow:
		case kBear:
		case kCalendar:
		case kVase1:
		case kVase2:
		case kBulletin:
		case kCloud:
		case kFaucet:
		case kRug:
		case kChimes:
		DoFurnitureObjectInfo();
		break;
		
		case kGreaseRt:
		case kGreaseLf:
		DoGreaseObjectInfo();
		break;
		
		case kInvisBonus:
		DoInvisBonusObjectInfo();
		break;
		
		case kMailboxLf:
		case kMailboxRt:
		case kFloorTrans:
		case kCeilingTrans:
		case kInvisTrans:
		case kDeluxeTrans:
		DoTransObjectInfo(thisRoom->objects[objActive].what);
		break;
		
		case kLightSwitch:
		case kMachineSwitch:
		case kThermostat:
		case kPowerSwitch:
		case kKnifeSwitch:
		case kInvisSwitch:
		DoSwitchObjectInfo();
		break;
		
		case kTrigger:
		case kLgTrigger:
		DoTriggerObjectInfo();
		break;
		
		case kCeilingLight:
		case kLightBulb:
		case kTableLamp:
		case kHipLamp:
		case kDecoLamp:
		case kFlourescent:
		case kTrackLight:
		case kInvisLight:
		DoLightObjectInfo();
		break;
		
		case kShredder:
		case kToaster:
		case kMacPlus:
		case kTV:
		case kCoffee:
		case kOutlet:
		case kVCR:
		DoApplianceObjectInfo(thisRoom->objects[objActive].what);
		break;
		
		case kMicrowave:
		DoMicrowaveObjectInfo();
		break;
		
		case kBalloon:
		case kCopterLf:
		case kCopterRt:
		case kDartLf:
		case kDartRt:
		case kBall:
		case kDrip:
		case kFish:
		DoEnemyObjectInfo(thisRoom->objects[objActive].what);
		break;
		
		case kFlower:
		DoFlowerObjectInfo();
		break;
		
		case kSoundTrigger:
		case kCustomPict:
		DoCustPictObjectInfo();
		break;
		
		default:
		SysBeep(1);
		break;
	}
}

#endif

PL_IMPLEMENT_FILTER_FUNCTION(BlowerFilter)
PL_IMPLEMENT_FILTER_FUNCTION(FurnitureFilter)
PL_IMPLEMENT_FILTER_FUNCTION(CustPictFilter)
PL_IMPLEMENT_FILTER_FUNCTION(SwitchFilter)
PL_IMPLEMENT_FILTER_FUNCTION(TriggerFilter)
PL_IMPLEMENT_FILTER_FUNCTION(LightFilter)
PL_IMPLEMENT_FILTER_FUNCTION(ApplianceFilter)
PL_IMPLEMENT_FILTER_FUNCTION(MicrowaveFilter)
PL_IMPLEMENT_FILTER_FUNCTION(GreaseFilter)
PL_IMPLEMENT_FILTER_FUNCTION(InvisBonusFilter)
PL_IMPLEMENT_FILTER_FUNCTION(TransFilter)
PL_IMPLEMENT_FILTER_FUNCTION(EnemyFilter)
PL_IMPLEMENT_FILTER_FUNCTION(FlowerFilter)

