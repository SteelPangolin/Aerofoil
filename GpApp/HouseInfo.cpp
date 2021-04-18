
//============================================================================
//----------------------------------------------------------------------------
//								  HouseInfo.c
//----------------------------------------------------------------------------
//============================================================================


#include "Externs.h"
#include "DialogManager.h"
#include "DialogUtils.h"
#include "IGpDisplayDriver.h"

#include "PLArrayView.h"
#include "PLDrivers.h"
#include "PLEditboxWidget.h"
#include "PLNumberFormatting.h"
#include "PLKeyEncoding.h"
#include "PLPasStr.h"
#include "PLTimeTaggedVOSEvent.h"


#define kHouseInfoDialogID		1001
#define kBannerTextItem			4
#define kLockHouseButton		6
#define kClearScoresButton		9
#define kTrailerTextItem		11
#define kNoPhoneCheck			14
#define kBannerNCharsItem		15
#define kTrailerNCharsItem		16
#define kHouseSizeItem			18
#define kLockHouseAlert			1029
#define kZeroScoresAlert		1032


long CountTotalHousePoints (void);
void UpdateHouseInfoDialog (Dialog *);
int16_t HouseFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt);
Boolean WarnLockingHouse (void);
void HowToZeroScores (void);


Str255		banner, trailer;
Rect		houseEditText1, houseEditText2;
short		houseCursorIs;
Boolean		keyHit, tempPhoneBit;

extern	Boolean		noRoomAtAll, changeLockStateOfHouse, saveHouseLocked;
extern	Boolean		phoneBitSet;

#ifndef COMPILEDEMO


//==============================================================  Functions
//--------------------------------------------------------------  CountTotalHousePoints

// The following functions all handle the "House Info" dialog in the editor.

long CountTotalHousePoints (void)
{
	long		pointTotal;
	short		numRooms, h, i;
	char		wasState;
	
	pointTotal = (long)RealRoomNumberCount() * (long)kRoomVisitScore;
	
	numRooms = (*thisHouse)->nRooms;
	for (i = 0; i < numRooms; i++)
	{
		if ((*thisHouse)->rooms[i].suite != kRoomIsEmpty)
		{
			for (h = 0; h < kMaxRoomObs; h++)
			{
				switch ((*thisHouse)->rooms[i].objects[h].what)
				{
					case kRedClock:
					pointTotal += kRedClockPoints;
					break;
					
					case kBlueClock:
					pointTotal += kBlueClockPoints;
					break;
					
					case kYellowClock:
					pointTotal += kYellowClockPoints;
					break;
					
					case kCuckoo:
					pointTotal += kCuckooClockPoints;
					break;
					
					case kStar:
					pointTotal += kStarPoints;
					break;
					
					case kInvisBonus:
					pointTotal += (*thisHouse)->rooms[i].objects[h].data.c.points;
					break;
					
					default:
					break;
				}
			}
		}
	}
	
	return (pointTotal);
}

//--------------------------------------------------------------  UpdateHouseInfoDialog

void UpdateHouseInfoDialog (Dialog *theDialog)
{
	short		nChars;
	
	nChars = GetDialogStringLen(theDialog, kBannerTextItem);
	SetDialogNumToStr(theDialog, kBannerNCharsItem, (long)nChars);
	nChars = GetDialogStringLen(theDialog, kTrailerTextItem);
	SetDialogNumToStr(theDialog, kTrailerNCharsItem, (long)nChars);
	SetDialogNumToStr(theDialog, kHouseSizeItem, CountTotalHousePoints());
	FrameDialogItemC(theDialog, 10, kRedOrangeColor8);
	SetDialogItemValue(theDialog, kNoPhoneCheck, (short)tempPhoneBit);
}

//--------------------------------------------------------------  HouseFilter

int16_t HouseFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	Point		mouseIs;
	short		nChars;
	
	if (keyHit)
	{
		nChars = GetDialogStringLen(dial, kBannerTextItem);
		SetDialogNumToStr(dial, kBannerNCharsItem, (long)nChars);
		nChars = GetDialogStringLen(dial, kTrailerTextItem);
		SetDialogNumToStr(dial, kTrailerNCharsItem, (long)nChars);
		keyHit = false;
	}

	if (evt)
	{
		if (evt->m_vosEvent.m_eventType == GpVOSEventTypes::kKeyboardInput)
		{
			const GpKeyboardInputEvent &keyEvt = evt->m_vosEvent.m_event.m_keyboardInputEvent;

			if (keyEvt.m_eventType == GpKeyboardInputEventTypes::kDown)
			{
				switch (PackVOSKeyCode(keyEvt))
				{
				case PL_KEY_NUMPAD_SPECIAL(kEnter):
					FlashDialogButton(dial, kOkayButton);
					return kOkayButton;

				case PL_KEY_SPECIAL(kEscape):
					FlashDialogButton(dial, kCancelButton);
					return kCancelButton;

				default:
					keyHit = true;
					return -1;
				}
			}
			else if (keyEvt.m_eventType == GpKeyboardInputEventTypes::kDownChar || keyEvt.m_eventType == GpKeyboardInputEventTypes::kAutoChar)
			{
				keyHit = true;
				return -1;
			}
		}
		else if (evt->m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
		{
			const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;

			if (mouseEvt.m_eventType == GpMouseEventTypes::kMove)
			{
				mouseIs = Point::Create(mouseEvt.m_x, mouseEvt.m_y);
				mouseIs -= dial->GetWindow()->GetTopLeftCoord();
				if ((houseEditText1.Contains(mouseIs)) ||
					(houseEditText2.Contains(mouseIs)))
				{
					if (houseCursorIs != kBeamCursor)
					{
						PLDrivers::GetDisplayDriver()->SetStandardCursor(EGpStandardCursors::kIBeam);
						houseCursorIs = kBeamCursor;
					}
				}
				else
				{
					if (houseCursorIs != kArrowCursor)
					{
						InitCursor();
						houseCursorIs = kArrowCursor;
					}
				}
			}
		}
	}

	return -1;
}

//--------------------------------------------------------------  DoHouseInfo

void DoHouseInfo (void)
{
	Dialog			*houseInfoDialog;
	Str255			versStr, loVers, nRoomsStr;
	long			h, v;
	short			item, numRooms, version;
	char			wasState;
	Boolean			leaving;

	tempPhoneBit = phoneBitSet;
	
	numRooms = RealRoomNumberCount();
	PasStringCopy((*thisHouse)->banner, banner);
	PasStringCopy((*thisHouse)->trailer, trailer);
	version = (*thisHouse)->version;
	if (!noRoomAtAll)
	{
		h = (long)(*thisHouse)->rooms[(*thisHouse)->firstRoom].suite;
		v = (long)(*thisHouse)->rooms[(*thisHouse)->firstRoom].floor;
	}
	
	NumToString((long)version >> 8, versStr);		// Convert version to two stringsÉ
	NumToString((long)version % 0x0100, loVers);	// the 1's and 1/10th's part.
	NumToString((long)numRooms, nRoomsStr);			// Number of rooms -> string.

	DialogTextSubstitutions substitutions(versStr, loVers, nRoomsStr);
	
//	CenterDialog(kHouseInfoDialogID);
	houseInfoDialog = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(kHouseInfoDialogID, kPutInFront, &substitutions);
	if (houseInfoDialog == nil)
		RedAlert(kErrDialogDidntLoad);
	ShowWindow(houseInfoDialog->GetWindow());

	static_cast<PortabilityLayer::EditboxWidget*>(houseInfoDialog->GetItems()[kBannerTextItem - 1].GetWidget())->SetMultiLine(true);
	static_cast<PortabilityLayer::EditboxWidget*>(houseInfoDialog->GetItems()[kTrailerTextItem - 1].GetWidget())->SetMultiLine(true);

	SetDialogString(houseInfoDialog, kBannerTextItem, banner);
	SetDialogString(houseInfoDialog, kTrailerTextItem, trailer);
	SelectDialogItemText(houseInfoDialog, kBannerTextItem, 0, 1024);
	GetDialogItemRect(houseInfoDialog, kBannerTextItem, &houseEditText1);
	GetDialogItemRect(houseInfoDialog, kTrailerTextItem, &houseEditText2);
	houseCursorIs = kArrowCursor;
	leaving = false;

	UpdateHouseInfoDialog(houseInfoDialog);
	
	while (!leaving)
	{
		item = houseInfoDialog->ExecuteModal(nullptr, PL_FILTER_FUNC(HouseFilter));
		
		if (item == kOkayButton)
		{
			GetDialogString(houseInfoDialog, kBannerTextItem, banner);
			GetDialogString(houseInfoDialog, kTrailerTextItem, trailer);
			
			PasStringCopyNum(banner, (*thisHouse)->banner, 255);
			PasStringCopyNum(trailer, (*thisHouse)->trailer, 255);
			if (tempPhoneBit != phoneBitSet)
			{
				phoneBitSet = tempPhoneBit;
				if (phoneBitSet)
					(*thisHouse)->flags = (*thisHouse)->flags | 0x00000002;
				else
					(*thisHouse)->flags = (*thisHouse)->flags & 0xFFFFDFFD;
			}
			
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
			leaving = true;
		else if (item == kLockHouseButton)
		{
			if (WarnLockingHouse())
			{
				changeLockStateOfHouse = true;
				saveHouseLocked = true;
				fileDirty = true;
				UpdateMenus(false);
			}
		}
		else if (item == kClearScoresButton)
			HowToZeroScores();
		else if (item == kNoPhoneCheck)
		{
			tempPhoneBit = !tempPhoneBit;
			SetDialogItemValue(houseInfoDialog, kNoPhoneCheck, (short)tempPhoneBit);
		}
	}
	InitCursor();
	houseInfoDialog->Destroy();
}

//--------------------------------------------------------------  WarnLockingHouse

Boolean WarnLockingHouse (void)
{
	short		hitWhat;
	
//	CenterAlert(kLockHouseAlert);
	hitWhat = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kLockHouseAlert, nullptr);
	
	return (hitWhat == 1);
}

//--------------------------------------------------------------  HowToZeroScores

void HowToZeroScores (void)
{
	short		hitWhat;
	
//	CenterAlert(kZeroScoresAlert);
	hitWhat = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kZeroScoresAlert, nullptr);
	
	switch (hitWhat)
	{
		case 2:		// zero all
		ZeroHighScores();
		fileDirty = true;
		UpdateMenus(false);
		break;
		
		case 3:		// zero all but highest
		ZeroAllButHighestScore();
		fileDirty = true;
		UpdateMenus(false);
		break;
	}
}

#endif

PL_IMPLEMENT_FILTER_FUNCTION(HouseFilter)
