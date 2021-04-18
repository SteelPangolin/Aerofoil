
//============================================================================
//----------------------------------------------------------------------------
//								   RoomInfo.c
//----------------------------------------------------------------------------
//============================================================================

#include "DialogManager.h"
#include "DialogUtils.h"
#include "Externs.h"
#include "IGpDisplayDriver.h"
#include "RectUtils.h"
#include "PLPopupMenuWidget.h"
#include "PLTimeTaggedVOSEvent.h"
#include "QDPixMap.h"
#include "ResolveCachingColor.h"
#include "ResourceCompiledRef.h"
#include "ResourceManager.h"
#include "Utilities.h"

#include "PLDrivers.h"
#include "PLKeyEncoding.h"
#include "PLNumberFormatting.h"
#include "PLResources.h"
#include "PLSound.h"
#include "PLPasStr.h"
#include "PLStandardColors.h"

#define kRoomInfoDialogID			1003
#define kOriginalArtDialogID		1016
#define kNoPICTFoundAlert			1036
#define kRoomNameItem				3
#define kRoomLocationBox			6
#define kRoomTilesBox				10
#define kRoomPopupItem				11
#define kRoomDividerLine			12
#define kRoomTilesBox2				15
#define kRoomFirstCheck				17
#define kLitUnlitText				18
#define kMiniTileWide				16
#define kBoundsButton				19
#define kOriginalArtworkItem		19
#define kPICTIDItem					5
#define kFloorSupportCheck			12


void UpdateRoomInfoDialog (Dialog *);
void DragMiniTile (Window *, DrawSurface *, Point, short *);
void HiliteTileOver (DrawSurface *, Point);
int16_t RoomFilter (void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt);

short ChooseOriginalArt (short);
void UpdateOriginalArt (Dialog *);
int16_t OriginalArtFilter (void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt);
Boolean PictIDExists (short);
short GetFirstPICT (void);
void ComplainAboutPICTNotFound (void);


Rect		tileSrc, tileDest, tileSrcRect, editTETextBox;
Rect		leftBound, topBound, rightBound, bottomBound;
DrawSurface	*tileSrcMap;
short		tempTiles[kNumTiles];
short		tileOver, tempBack, cursorIs;
Boolean		originalLeftOpen, originalTopOpen, originalRightOpen, originalBottomOpen;
Boolean		originalFloor;

extern	IGpCursor	*handCursor;
extern	short		lastBackground;

extern PortabilityLayer::IResourceArchive *houseResFork;


//==============================================================  Functions
//--------------------------------------------------------------  UpdateRoomInfoDialog

#ifndef COMPILEDEMO
void UpdateRoomInfoDialog (Dialog *theDialog)
{
	Rect		src, dest;
	short		i;
	
	if (tempBack >= kUserBackground)
		SetPopUpMenuValue(theDialog, kRoomPopupItem, kOriginalArtworkItem);
	else
		SetPopUpMenuValue(theDialog, kRoomPopupItem, 
				(tempBack - kBaseBackgroundID) + 1);
	
	
	
	CopyBits(GetPortBitMapForCopyBits(tileSrcMap), 
			GetPortBitMapForCopyBits(theDialog->GetWindow()->GetDrawSurface()),
			&tileSrcRect, &tileSrc, srcCopy);
	/*
	CopyBits(&((GrafPtr)tileSrcMap)->portBits, 
			&(((GrafPtr)theDialog)->portBits), 
			&tileSrcRect, &tileSrc, srcCopy, nil);
	*/
	dest = tileDest;
	dest.right = dest.left + kMiniTileWide;
	for (i = 0; i < kNumTiles; i++)
	{
		QSetRect(&src, 0, 0, kMiniTileWide, 80);
		QOffsetRect(&src, tempTiles[i] * kMiniTileWide, 0);
		
		CopyBits(GetPortBitMapForCopyBits(tileSrcMap), 
				GetPortBitMapForCopyBits(theDialog->GetWindow()->GetDrawSurface()),
				&src, &dest, srcCopy);
		/*
		CopyBits(&((GrafPtr)tileSrcMap)->portBits, 
				&(((GrafPtr)theDialog)->portBits), 
				&src, &dest, srcCopy, nil);
		*/
		QOffsetRect(&dest, kMiniTileWide, 0);
	}
	
	if (GetNumberOfLights(thisRoomNumber) == 0)
		SetDialogString(theDialog, kLitUnlitText, PSTR("(Room Is Dark)"));
	else
		SetDialogString(theDialog, kLitUnlitText, PSTR("(Room Is Lit)"));

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	
	FrameDialogItemC(theDialog, kRoomLocationBox, kRedOrangeColor8);
	FrameDialogItem(theDialog, kRoomTilesBox, blackColor);
	FrameDialogItemC(theDialog, kRoomDividerLine, kRedOrangeColor8);
	FrameDialogItem(theDialog, kRoomTilesBox2, blackColor);
}
#endif

//--------------------------------------------------------------  DragMiniTile

#ifndef COMPILEDEMO
void DragMiniTile (Window *window, DrawSurface *surface, Point mouseIs, short *newTileOver)
{
	Rect		dragRect;
	Point		mouseWas;
	short		wasTileOver;
	Pattern		dummyPattern;

	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	PortabilityLayer::ResolveCachingColor blueColor = StdColors::Blue();
	
	tileOver = (mouseIs.h - tileSrc.left) / kMiniTileWide;
	wasTileOver = -1;
	QSetRect(&dragRect, 0, 0, kMiniTileWide, 80);
	QOffsetRect(&dragRect, 
			tileSrc.left + (tileOver * kMiniTileWide), 
			tileSrc.top);

	const uint8_t *pattern = *GetQDGlobalsGray(&dummyPattern);

	surface->InvertFrameRect(dragRect, pattern);

	mouseWas = mouseIs;
	while (WaitMouseUp())							// loop until mouse button let up
	{
		GetMouse(window, &mouseIs);							// get mouse coords
		if (mouseWas != mouseIs)					// the mouse has moved
		{
			surface->InvertFrameRect(dragRect, pattern);
			QOffsetRect(&dragRect, mouseIs.h - mouseWas.h, 0);
			surface->InvertFrameRect(dragRect, pattern);

			if (tileDest.Contains(mouseIs))		// is cursor in the drop rect
			{
				*newTileOver = (mouseIs.h - tileDest.left) / kMiniTileWide;
				if (*newTileOver != wasTileOver)
				{

					for (int offset = 0; offset < 2; offset++)
					{
						Point pointA = Point::Create(tileDest.left + (*newTileOver * kMiniTileWide), tileDest.top - 3 + offset);
						Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

						surface->DrawLine(pointA, pointB, blueColor);
					}

					for (int offset = 0; offset < 2; offset++)
					{
						Point pointA = Point::Create(tileDest.left + (*newTileOver * kMiniTileWide), tileDest.bottom + 1 + offset);
						Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

						surface->DrawLine(pointA, pointB, blueColor);
					}
					
					if (wasTileOver != -1)
					{
						for (int offset = 0; offset < 2; offset++)
						{
							Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.top - 3 + offset);
							Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

							surface->DrawLine(pointA, pointB, whiteColor);
						}

						for (int offset = 0; offset < 2; offset++)
						{
							Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.bottom + 1 + offset);
							Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

							surface->DrawLine(pointA, pointB, whiteColor);
						}
					}

					wasTileOver = *newTileOver;
				}
			}
			else
			{
				*newTileOver = -1;					// we're not in the drop zone
				if (wasTileOver != -1)
				{
					for (int offset = 0; offset < 2; offset++)
					{
						Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.top - 3 + offset);
						Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

						surface->DrawLine(pointA, pointB, whiteColor);
					}

					for (int offset = 0; offset < 2; offset++)
					{
						Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.bottom + 1 + offset);
						Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

						surface->DrawLine(pointA, pointB, whiteColor);
					}

					wasTileOver = -1;
				}
			}
			
			mouseWas = mouseIs;
		}
	}
	if (wasTileOver != -1)
	{
		for (int offset = 0; offset < 2; offset++)
		{
			Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.top - 3 + offset);
			Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

			surface->DrawLine(pointA, pointB, whiteColor);
		}

		for (int offset = 0; offset < 2; offset++)
		{
			Point pointA = Point::Create(tileDest.left + (wasTileOver * kMiniTileWide), tileDest.bottom + 1 + offset);
			Point pointB = Point::Create(pointA.h + kMiniTileWide, pointA.v);

			surface->DrawLine(pointA, pointB, whiteColor);
		}

		wasTileOver = -1;
	}
	surface->InvertFrameRect(dragRect, pattern);
}
#endif

//--------------------------------------------------------------  HiliteTileOver

#ifndef COMPILEDEMO
void HiliteTileOver (DrawSurface *surface, Point mouseIs)
{
	short		newTileOver;
	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	PortabilityLayer::ResolveCachingColor redColor = StdColors::Red();
	
	if (tileSrc.Contains(mouseIs))
	{
		if (cursorIs != kHandCursor)
		{
			PLDrivers::GetDisplayDriver()->SetCursor(handCursor);
			cursorIs = kHandCursor;
		}
		
		newTileOver = (mouseIs.h - tileSrc.left) / kMiniTileWide;
		if (newTileOver != tileOver)
		{
			{
				const Point tileLineTopLeft = Point::Create(tileSrc.left + (newTileOver * kMiniTileWide), tileSrc.top - 3);
				const Point tileLineBottomRight = Point::Create(tileLineTopLeft.h + kMiniTileWide + 1, tileLineTopLeft.v + 2);
				surface->FillRect(Rect::Create(tileLineTopLeft.v, tileLineTopLeft.h, tileLineBottomRight.v, tileLineBottomRight.h), redColor);
			}
			
			if (tileOver != -1)
			{
				{
					const Point tileLineTopLeft = Point::Create(tileSrc.left + (tileOver * kMiniTileWide), tileSrc.top - 3);
					const Point tileLineBottomRight = Point::Create(tileLineTopLeft.h + kMiniTileWide + 1, tileLineTopLeft.v + 2);
					surface->FillRect(Rect::Create(tileLineTopLeft.v, tileLineTopLeft.h, tileLineBottomRight.v, tileLineBottomRight.h), whiteColor);
				}

				{
					const Point tileLineTopLeft = Point::Create(tileSrc.left + (tileOver * kMiniTileWide), tileSrc.bottom + 1);
					const Point tileLineBottomRight = Point::Create(tileLineTopLeft.h + kMiniTileWide + 1, tileLineTopLeft.v + 2);
					surface->FillRect(Rect::Create(tileLineTopLeft.v, tileLineTopLeft.h, tileLineBottomRight.v, tileLineBottomRight.h), whiteColor);
				}
			}

			tileOver = newTileOver;
		}
	}
	else
	{
		if (tileOver != -1)
		{
			{
				const Point tileLineTopLeft = Point::Create(tileSrc.left + (tileOver * kMiniTileWide), tileSrc.top - 3);
				const Point tileLineBottomRight = Point::Create(tileLineTopLeft.h + kMiniTileWide + 1, tileLineTopLeft.v + 2);
				surface->FillRect(Rect::Create(tileLineTopLeft.v, tileLineTopLeft.h, tileLineBottomRight.v, tileLineBottomRight.h), whiteColor);
			}

			{
				const Point tileLineTopLeft = Point::Create(tileSrc.left + (tileOver * kMiniTileWide), tileSrc.bottom + 1);
				const Point tileLineBottomRight = Point::Create(tileLineTopLeft.h + kMiniTileWide + 1, tileLineTopLeft.v + 2);
				surface->FillRect(Rect::Create(tileLineTopLeft.v, tileLineTopLeft.h, tileLineBottomRight.v, tileLineBottomRight.h), whiteColor);
			}

			tileOver = -1;
		}
		
		if (editTETextBox.Contains(mouseIs))
		{
			if (cursorIs != kBeamCursor)
			{
				PLDrivers::GetDisplayDriver()->SetStandardCursor(EGpStandardCursors::kIBeam);
				cursorIs = kBeamCursor;
			}
		}
		else
		{
			if (cursorIs != kArrowCursor)
			{
				InitCursor();
				cursorIs = kArrowCursor;
			}
		}
	}
}
#endif

//--------------------------------------------------------------  RoomFilter
#ifndef COMPILEDEMO

int16_t RoomFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	Point		mouseIs;
	short		newTileOver;

	if (!evt)
		return -1;

	Window *window = dial->GetWindow();
	DrawSurface *surface = dial->GetWindow()->GetDrawSurface();

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
			SelectDialogItemText(dial, kRoomNameItem, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}
	else if (evt->m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
	{
		const GpMouseInputEvent &mouseEvent = evt->m_vosEvent.m_event.m_mouseInputEvent;

		if (evt->IsLMouseDownEvent())
		{
			mouseIs = Point::Create(mouseEvent.m_x, mouseEvent.m_y);
			mouseIs -= dial->GetWindow()->GetTopLeftCoord();
			if (tileSrc.Contains(mouseIs))
			{
				if (StillDown())
				{
					DragMiniTile(window, surface, mouseIs, &newTileOver);
					if ((newTileOver >= 0) && (newTileOver < kNumTiles))
					{
						tempTiles[newTileOver] = tileOver;
						UpdateRoomInfoDialog(dial);
					}
				}
				return 0;
			}
			else
				return -1;
		}
		else if (mouseEvent.m_eventType == GpMouseEventTypes::kMove)
		{
			mouseIs = dial->GetWindow()->MouseToLocal(mouseEvent);
			HiliteTileOver(surface, mouseIs);

			return -1;
		}
	}

	return -1;
}
#endif

//--------------------------------------------------------------  DoRoomInfo

void DoRoomInfo(void)
{
#ifndef COMPILEDEMO
#define			kBackgroundsMenuID		140
	Dialog			*roomInfoDialog;
	Str255			floorStr, suiteStr, objectsStr, tempStr;
	short			item, i, newBack;
	char			wasState;
	Boolean			leaving, wasFirstRoom, forceDraw;
	PLError_t		theErr;

	tileOver = -1;
	cursorIs = kArrowCursor;
	tempBack = thisRoom->background;
	//	SetMenuItemTextStyle(backgroundsMenu, kOriginalArtworkItem, italic);

	NumToString(thisRoom->floor, floorStr);
	NumToString(thisRoom->suite, suiteStr);
	NumToString(thisRoom->numObjects, objectsStr);
	DialogTextSubstitutions substitutions(floorStr, suiteStr, objectsStr);

	theErr = CreateOffScreenGWorld(&tileSrcMap, &tileSrcRect);
	//	CreateOffScreenPixMap(&tileSrcRect, &tileSrcMap);
	//	SetPort((GrafPtr)tileSrcMap);
	if ((tempBack > kStars) && (!PictIDExists(tempBack)))
	{
		ComplainAboutPICTNotFound();
		tempBack = kSimpleRoom;
	}
	if ((tempBack == 2002) || (tempBack == 2011) ||
		(tempBack == 2016) || (tempBack == 2017))
		LoadScaledGraphicCustom(tileSrcMap, tempBack - 800, &tileSrcRect);
	else
		LoadScaledGraphicCustom(tileSrcMap, tempBack, &tileSrcRect);

	for (i = 0; i < kNumTiles; i++)
		tempTiles[i] = thisRoom->tiles[i];

	//	CenterDialog(kRoomInfoDialogID);
	roomInfoDialog = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(kRoomInfoDialogID, kPutInFront, &substitutions);
	if (roomInfoDialog == nil)
		RedAlert(kErrDialogDidntLoad);

	{
		PortabilityLayer::WidgetBasicState state;

		GetDialogItemRect(roomInfoDialog, kRoomPopupItem, &state.m_rect);
		state.m_resID = kBackgroundsMenuID;
		state.m_enabled = true;

		PortabilityLayer::PopupMenuWidget *roomPopupWidget = PortabilityLayer::PopupMenuWidget::Create(state, nullptr);
		roomInfoDialog->ReplaceWidget(kRoomPopupItem - 1, roomPopupWidget);

		if (HouseHasOriginalPicts())
			EnableMenuItem(roomPopupWidget->GetMenu(), kOriginalArtworkItem);
	}

	if (tempBack >= kUserBackground)
		SetPopUpMenuValue(roomInfoDialog, kRoomPopupItem, kOriginalArtworkItem);
	else
		SetPopUpMenuValue(roomInfoDialog, kRoomPopupItem, 
				(tempBack - kBaseBackgroundID) + 1);
	SetDialogString(roomInfoDialog, kRoomNameItem, thisRoom->name);
	GetDialogItemRect(roomInfoDialog, kRoomTilesBox, &tileSrc);
	GetDialogItemRect(roomInfoDialog, kRoomTilesBox2, &tileDest);
	GetDialogItemRect(roomInfoDialog, kRoomNameItem, &editTETextBox);
	SelectDialogItemText(roomInfoDialog, kRoomNameItem, 0, 1024);
	
	ShowWindow(roomInfoDialog->GetWindow());
	DrawDefaultButton(roomInfoDialog);
	
	wasFirstRoom = ((*thisHouse)->firstRoom == thisRoomNumber);
	SetDialogItemValue(roomInfoDialog, kRoomFirstCheck, (short)wasFirstRoom);
	
	if (tempBack >= kUserBackground)
		MyEnableControl(roomInfoDialog, kBoundsButton);
	else
		MyDisableControl(roomInfoDialog, kBoundsButton);
	
	leaving = false;

	UpdateRoomInfoDialog(roomInfoDialog);
	
	while (!leaving)
	{
		bool needRedraw = false;

		item = roomInfoDialog->ExecuteModal(nullptr, PL_FILTER_FUNC(RoomFilter));
		
		if (item == kOkayButton)
		{
			for (i = 0; i < kNumTiles; i++)
				thisRoom->tiles[i] = tempTiles[i];
			
			GetDialogString(roomInfoDialog, kRoomNameItem, tempStr);
			PasStringCopyNum(tempStr, thisRoom->name, 27);
			if (wasFirstRoom)
			{
				(*thisHouse)->firstRoom = thisRoomNumber;
			}
			thisRoom->background = tempBack;
			if (tempBack < kUserBackground)
				lastBackground = tempBack;
			CopyThisRoomToRoom();
			ReflectCurrentRoom(false);
			fileDirty = true;
			UpdateMenus(false);
			leaving = true;
		}
		else if (item == kCancelButton)
		{
			leaving = true;
		}
		else if (item == kRoomFirstCheck)
		{
			wasFirstRoom = !wasFirstRoom;
			SetDialogItemValue(roomInfoDialog, kRoomFirstCheck, (short)wasFirstRoom);
		}
		else if (item == kRoomPopupItem)
		{
			GetPopUpMenuValue(roomInfoDialog, kRoomPopupItem, &newBack);
			if (newBack == kOriginalArtworkItem)				// original art item selected?
			{
				if (tempBack < kUserBackground)					// was previous bg built-in?
				{
					tempBack = GetFirstPICT();					// then assign 1st PICT
					forceDraw = true;
				}
				else
					forceDraw = false;
				newBack = ChooseOriginalArt(tempBack);			// bring up dialog
				if ((tempBack != newBack) || (forceDraw))
				{
					tempBack = newBack;
					LoadScaledGraphicCustom(tileSrcMap, tempBack, &tileSrcRect);
					needRedraw = true;
				}
			}
			else
			{
				newBack += (kBaseBackgroundID - 1);		// adjust to get real PICT ID
				if (newBack != tempBack)				// if background has changed
					SetInitialTiles(newBack, false);
			}
			
			if (newBack >= kUserBackground)
			{
				MyEnableControl(roomInfoDialog, kBoundsButton);
				if (newBack != tempBack)				// if background has changed
					SetInitialTiles(newBack, false);
			}
			else
				MyDisableControl(roomInfoDialog, kBoundsButton);
			
			if (newBack != tempBack)
			{
				tempBack = newBack;
				if ((tempBack == 2002) || (tempBack == 2011) || 
						(tempBack == 2016) || (tempBack == 2017))
					LoadScaledGraphicCustom(tileSrcMap, tempBack - 800, &tileSrcRect);
				else
					LoadScaledGraphicCustom(tileSrcMap, tempBack, &tileSrcRect);
				needRedraw = true;
			}
		}
		else if (item == kBoundsButton)
		{
			newBack = ChooseOriginalArt(tempBack);
			if (tempBack != newBack)
			{
				tempBack = newBack;
				LoadScaledGraphicCustom(tileSrcMap, tempBack, &tileSrcRect);
				needRedraw = true;
			}
		}

		if (needRedraw)
			UpdateRoomInfoDialog(roomInfoDialog);
	}
	
	InitCursor();
	roomInfoDialog->Destroy();
	
//	KillOffScreenPixMap(tileSrcMap);
	DisposeGWorld(tileSrcMap);
	tileSrcMap = nil;
#endif
}

//--------------------------------------------------------------  UpdateOriginalArt

#ifndef COMPILEDEMO
void UpdateOriginalArt (Dialog *theDialog)
{
	Pattern		grayPattern;
	GetQDGlobalsGray(&grayPattern);
	
	DrawDefaultButton(theDialog);
	
	if (!originalLeftOpen)
		BorderDialogItem(theDialog, 7, 8, 2, nullptr);
	else
		BorderDialogItem(theDialog, 7, 8, 2, grayPattern);
	
	if (!originalTopOpen)
		BorderDialogItem(theDialog, 8, 4, 2, nullptr);
	else
		BorderDialogItem(theDialog, 8, 4, 2, grayPattern);
	
	if (!originalRightOpen)
		BorderDialogItem(theDialog, 9, 1, 2, nullptr);
	else
		BorderDialogItem(theDialog, 9, 1, 2, grayPattern);
	
	if (!originalBottomOpen)
		BorderDialogItem(theDialog, 10, 2, 2, nullptr);
	else
		BorderDialogItem(theDialog, 10, 2, 2, grayPattern);
}
#endif

//--------------------------------------------------------------  OriginalArtFilter
#ifndef COMPILEDEMO

int16_t OriginalArtFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	Point		mouseIs;

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
			SelectDialogItemText(dial, kPICTIDItem, 0, 1024);
			return 0;

		default:
			return -1;
		}
	}
	else if (evt->IsLMouseDownEvent())
	{
		const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;

		mouseIs = Point::Create(mouseEvt.m_x, mouseEvt.m_y);
		mouseIs -= dial->GetWindow()->GetTopLeftCoord();
		if (leftBound.Contains(mouseIs))
			return 7;
		else if (topBound.Contains(mouseIs))
			return 8;
		else if (rightBound.Contains(mouseIs))
			return 9;
		else if (bottomBound.Contains(mouseIs))
			return 10;
		else
			return -1;
	}

	return -1;
}
#endif

//--------------------------------------------------------------  ChooseOriginalArt

#ifndef COMPILEDEMO
short ChooseOriginalArt (short was)
{
	Dialog			*theDialog;
	long			longID;
	short			item, newPictID, tempShort, wasPictID;
	Boolean			leaving;
	
	if (was < kUserBackground)
		was = kUserBackground;
	
	InitCursor();
	BringUpDialog(&theDialog, kOriginalArtDialogID, nullptr);
	if (was >= kOriginalArtworkItem)
	{
		newPictID = was;
		wasPictID = was;
	}
	else
	{
		newPictID = kUserBackground;
		wasPictID = 0;
	}
	SetDialogNumToStr(theDialog, kPICTIDItem, (long)newPictID);
	SelectDialogItemText(theDialog, kPICTIDItem, 0, 16);
	
	GetDialogItemRect(theDialog, 7, &leftBound);
	GetDialogItemRect(theDialog, 8, &topBound);
	GetDialogItemRect(theDialog, 9, &rightBound);
	GetDialogItemRect(theDialog, 10, &bottomBound);
	
	tempShort = thisRoom->bounds >> 1;			// version 2.0 house
	originalLeftOpen = ((tempShort & 1) == 1);
	originalTopOpen = ((tempShort & 2) == 2);
	originalRightOpen = ((tempShort & 4) == 4);
	originalBottomOpen = ((tempShort & 8) == 8);
	originalFloor = ((tempShort & 16) == 16);
	
	SetDialogItemValue(theDialog, kFloorSupportCheck, (short)originalFloor);
	
	leaving = false;

	UpdateOriginalArt(theDialog);
	
	while (!leaving)
	{
		item = theDialog->ExecuteModal(nullptr, PL_FILTER_FUNC(OriginalArtFilter));
		
		if (item == kOkayButton)
		{
			GetDialogNumFromStr(theDialog, kPICTIDItem, &longID);
			if ((longID >= 3000) && (longID < 3800) && (PictIDExists((short)longID)))
			{
				newPictID = (short)longID;
				if (newPictID != wasPictID)
					SetInitialTiles(tempBack, false);
				tempShort = 0;
				if (originalLeftOpen)
					tempShort += 1;
				if (originalTopOpen)
					tempShort += 2;
				if (originalRightOpen)
					tempShort += 4;
				if (originalBottomOpen)
					tempShort += 8;
				if (originalFloor)
					tempShort += 16;
				tempShort = tempShort << 1;		// shift left 1 bit
				tempShort += 1;					// flag that says orginal bounds used
				thisRoom->bounds = tempShort;
				leaving = true;
			}
			else
			{
				SysBeep(1);
				SetDialogNumToStr(theDialog, kPICTIDItem, (long)newPictID);
			}
		}
		else if (item == kCancelButton)
		{
			newPictID = was;
			leaving = true;
		}
		else if (item == 7)
		{
			originalLeftOpen = !originalLeftOpen;
			UpdateOriginalArt(theDialog);
		}
		else if (item == 8)
		{
			originalTopOpen = !originalTopOpen;
			UpdateOriginalArt(theDialog);
		}
		else if (item == 9)
		{
			originalRightOpen = !originalRightOpen;
			UpdateOriginalArt(theDialog);
		}
		else if (item == 10)
		{
			originalBottomOpen = !originalBottomOpen;
			UpdateOriginalArt(theDialog);
		}
		else if (item == kFloorSupportCheck)
		{
			originalFloor = !originalFloor;
			ToggleDialogItemValue(theDialog, kFloorSupportCheck);
		}
	}
	
	theDialog->Destroy();
	
	return (newPictID);
}
#endif

//--------------------------------------------------------------  PictIDExists

Boolean PictIDExists (short theID)
{
	THandle<void>	thePicture;
//	Handle		resHandle;
//	Str255		resName;
//	ResType		resType;
//	short		numPicts, i;
//	short		resID;
	Boolean		foundIt;
	
	foundIt = true;
	
	thePicture = LoadHouseResource('PICT', theID);
	if (thePicture == nil)
	{
		thePicture = LoadHouseResource('Date', theID);
		if (thePicture == nil)
		{
			foundIt = false;
		}
		else
			thePicture.Dispose();
	}
	else
		thePicture.Dispose();
	
//	foundIt = false;
//	numPicts = Count1Resources('PICT');
//	for (i = 1; i <= numPicts; i++)
//	{
//		resHandle = Get1IndResource('PICT', i);
//		if (resHandle != nil)
//		{
//			GetResInfo(resHandle, &resID, &resType, resName);
//			ReleaseResource(resHandle);
//			if (resID == theID)
//			{
//				foundIt = true;
//				break;
//			}
//		}
//	}
	
	return (foundIt);
}

//--------------------------------------------------------------  GetFirstPICT

short GetFirstPICT (void)
{
	Handle		resHandle;
	Str255		resName;

	int16_t resID = 0;
	if (!houseResFork->FindFirstResourceOfType('PICT', resID))
		return -1;

	return resID;
}

//--------------------------------------------------------------  ComplainAboutPICTNotFound

#ifndef COMPILEDEMO
void ComplainAboutPICTNotFound (void)
{
	short		hitWhat;
	
//	CenterAlert(kNoPICTFoundAlert);
	hitWhat = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kNoPICTFoundAlert, nullptr);
}
#endif

PL_IMPLEMENT_FILTER_FUNCTION(RoomFilter)
PL_IMPLEMENT_FILTER_FUNCTION(OriginalArtFilter)
