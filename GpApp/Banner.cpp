//============================================================================
//----------------------------------------------------------------------------
//									Banner.c
//----------------------------------------------------------------------------
//============================================================================


#include "PLNumberFormatting.h"
#include "PLPasStr.h"
#include "PLStandardColors.h"
#include "Externs.h"
#include "Environ.h"
#include "FontFamily.h"
#include "FontManager.h"
#include "MainWindow.h"
#include "RectUtils.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "ResolveCachingColor.h"
#include "Room.h"
#include "Utilities.h"
#include "WindowDef.h"
#include "WindowManager.h"
#include "PLSysCalls.h"


#define kBannerPageTopPICT		1993
#define kBannerPageBottomPICT	1992
#define kBannerPageBottomMask	1991
#define kStarsRemainingPICT		1017
#define kStarRemainingPICT		1018


void DrawBanner (Point *);
void DrawBannerMessage (Point);


short		numStarsRemaining;
Boolean		bannerStarCountOn;

extern	Rect		justRoomsRect;
extern	Boolean		quickerTransitions, demoGoing, isUseSecondScreen;


//==============================================================  Functions
//--------------------------------------------------------------  DrawBanner
// Displays opening banner (when a new game is begun).  The banner looksÉ
// like a sheet of notebook paper.  The text printed on it is specifiedÉ
// by the author of the house.

void DrawBanner (Point *topLeft)
{
	Rect		wholePage, partPage, mapBounds;
	DrawSurface *tempMap;
	DrawSurface *tempMask;
	PLError_t	theErr;
	
	QSetRect(&wholePage, 0, 0, 330, 220);
	mapBounds = thisMac.fullScreen;
	ZeroRectCorner(&mapBounds);
	CenterRectInRect(&wholePage, &mapBounds);
	topLeft->h = wholePage.left;
	topLeft->v = wholePage.top;
	partPage = wholePage;
	partPage.bottom = partPage.top + 190;	
	LoadScaledGraphicCustom(workSrcMap, kBannerPageTopPICT, &partPage);
	
	partPage = wholePage;
	partPage.top = partPage.bottom - 30;
	mapBounds = partPage;
	ZeroRectCorner(&mapBounds);
	theErr = CreateOffScreenGWorld(&tempMap, &mapBounds);
	LoadGraphicCustom(tempMap, kBannerPageBottomPICT);
	
	theErr = CreateOffScreenGWorldCustomDepth(&tempMask, &mapBounds, GpPixelFormats::kBW1);	
	LoadGraphicCustom(tempMask, kBannerPageBottomMask);

	CopyMask((BitMap *)*GetGWorldPixMap(tempMap), 
			(BitMap *)*GetGWorldPixMap(tempMask), 
			(BitMap *)*GetGWorldPixMap(workSrcMap), 
			&mapBounds, &mapBounds, &partPage);

	DisposeGWorld(tempMap);
	DisposeGWorld(tempMask);
}

//--------------------------------------------------------------  CountStarsInHouse
// Goes through the current house and counts the total number of stars within.

short CountStarsInHouse (void)
{
	short		i, h, numRooms, numStars;
	housePtr	housePtr = *thisHouse;
	
	numStars = 0;
	
	numRooms = housePtr->nRooms;
	for (i = 0; i < numRooms; i++)
	{
		if (housePtr->rooms[i].suite != kRoomIsEmpty)
			for (h = 0; h < kMaxRoomObs; h++)
			{
				if (housePtr->rooms[i].objects[h].what == kStar)
					numStars++;
			}
	}
	
	return (numStars);
}

//--------------------------------------------------------------  DrawBannerMessage

// This function prints the author's message acorss the notebook paper banner.

void DrawBannerMessage (Point topLeft)
{
	Str255		bannerStr, subStr;
	short		count;

	PasStringCopy((*thisHouse)->banner, bannerStr);

	PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication12Bold);

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

	count = 0;
	do
	{
		GetLineOfText(bannerStr, count, subStr);
		workSrcMap->DrawString(Point::Create(topLeft.h + 16, topLeft.v + 32 + (count * 20)), subStr, blackColor, appFont);
		count++;
	}
	while (subStr[0] > 0);
	
	if (bannerStarCountOn)
	{
		if (numStarsRemaining != 1)
			GetLocalizedString(1, bannerStr);
		else
			GetLocalizedString(2, bannerStr);
		
		NumToString((long)numStarsRemaining, subStr);
		PasStringConcat(bannerStr, subStr);
		
		if (numStarsRemaining != 1)
			GetLocalizedString(3, subStr);
		else
			GetLocalizedString(4, subStr);
		PasStringConcat(bannerStr, subStr);

		PortabilityLayer::ResolveCachingColor redColor = StdColors::Red();
		workSrcMap->DrawString(Point::Create(topLeft.h + 16, topLeft.v + 164), bannerStr, redColor, appFont);

		GetLocalizedString(5, subStr);
		workSrcMap->DrawString(Point::Create(topLeft.h + 16, topLeft.v + 180), subStr, redColor, appFont);
	}
}

//--------------------------------------------------------------  BringUpBanner
// Handles bringing up displaying and disposing of the banner.

void BringUpBanner (void)
{
	Rect		wholePage;
	Point		topLeft;
	
	DrawBanner(&topLeft);
	DrawBannerMessage(topLeft);

	DumpScreenOn(&justRoomsRect, false);

//	if (quickerTransitions)
//		DissBitsChunky(&justRoomsRect);		// was workSrcRect
//	else
//		DissBits(&justRoomsRect);
	QSetRect(&wholePage, 0, 0, 330, 220);
	QOffsetRect(&wholePage, topLeft.h, topLeft.v);
	
	CopyBits((BitMap *)*GetGWorldPixMap(backSrcMap), 
			(BitMap *)*GetGWorldPixMap(workSrcMap), 
			&wholePage, &wholePage, srcCopy);

	{
		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		if (demoGoing)
			WaitForInputEvent(4);
		else
			WaitForInputEvent(15);
	}
	
//	if (quickerTransitions)
//		DissBitsChunky(&justRoomsRect);
//	else
//		DissBits(&justRoomsRect);
}

//--------------------------------------------------------------  DisplayStarsRemaining
// This brings up a small message indicating the number of stars remainingÉ
// in a house.  It comes up when the player gets a star (the game is pausedÉ
// and the user informed as to how many remain).

void DisplayStarsRemaining(void)
{
	Rect		src, bounds;
	Str255		theStr;

	PortabilityLayer::WindowManager *wm = PortabilityLayer::WindowManager::GetInstance();

	QSetRect(&bounds, 0, 0, 256, 64);
	CenterRectInRect(&bounds, &thisMac.fullScreen);
	QOffsetRect(&bounds, -thisMac.fullScreen.left, -thisMac.fullScreen.top);
	QOffsetRect(&bounds, 0, -20);

	// Inset by 1 because the original graphic is bordered, and we want to use the window chrome for the border because it will apply the flicker effect
	Rect windowBounds = bounds.Inset(1, 1);

	PortabilityLayer::WindowDef wdef = PortabilityLayer::WindowDef::Create(windowBounds, 0, true, 0, 0, PSTR(""));

	// Zero out
	QOffsetRect(&bounds, -bounds.left - 1, -bounds.top - 1);

	Window *starsWindow = wm->CreateWindow(wdef);
	wm->PutWindowBehind(starsWindow, wm->GetPutInFrontSentinel());

	DrawSurface *surface = starsWindow->GetDrawSurface();

	PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication12Bold);

	NumToString((long)numStarsRemaining, theStr);

	if (numStarsRemaining < 2)
		LoadScaledGraphicCustom(surface, kStarRemainingPICT, &bounds);
	else
	{
		LoadScaledGraphicCustom(surface, kStarsRemainingPICT, &bounds);
		const Point textPoint = Point::Create(bounds.left + 102 - (appFont->MeasurePStr(theStr) / 2), bounds.top + 23);
		ColorText(surface, textPoint, theStr, 4L, appFont);
	}

	if (doZooms)
		wm->FlickerWindowIn(starsWindow, 32);

	{
		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		DelayTicks(60);
		WaitForInputEvent(30);
	}

	if (doZooms)
		wm->FlickerWindowOut(starsWindow, 32);

	wm->DestroyWindow(starsWindow);
}

