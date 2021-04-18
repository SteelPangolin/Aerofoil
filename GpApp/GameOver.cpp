//============================================================================
//----------------------------------------------------------------------------
//									GameOver.c
//----------------------------------------------------------------------------
//============================================================================


#include "PLPasStr.h"
#include "PLEventQueue.h"
#include "PLKeyEncoding.h"
#include "PLTimeTaggedVOSEvent.h"
#include "Externs.h"
#include "Environ.h"
#include "FontManager.h"
#include "FontFamily.h"
#include "InputManager.h"
#include "MainWindow.h"
#include "Objects.h"
#include "PLStandardColors.h"
#include "RectUtils.h"
#include "RenderedFont.h"
#include "ResolveCachingColor.h"
#include "Utilities.h"
#include "PLSysCalls.h"


#define kNumCountDownFrames		16
#define kPageFrames				14
#define kPagesPictID			1990
#define kPagesMaskID			1989
#define kLettersPictID			1988
#define kMilkywayPictID			1021


typedef struct
{
	Rect		dest, was;
	short		frame, counter;
	Boolean		stuck;
} pageType, *pagePtr;


void DoGameOverStarAnimation (void);
void SetUpFinalScreen (void);
void InitDiedGameOver (void);
void HandlePages (void);
void DrawPages (void);


pageType	pages[8];
Rect		pageSrcRect, pageSrc[kPageFrames], lettersSrc[8], angelSrcRect;
DrawSurface *pageSrcMap, *gameOverSrcMap, *angelSrcMap;
DrawSurface *pageMaskMap, *angelMaskMap;
short		countDown, stopPages, pagesStuck;
Boolean		gameOver;

extern	Rect		justRoomsRect;
extern	short		splashOriginH, splashOriginV, numWork2Main;
extern	short		numBack2Work;
extern	Boolean		playing, shadowVisible, demoGoing;


//==============================================================  Functions
//--------------------------------------------------------------  DoGameOver

// Handles a game over.  This is a game over where the player has…
// completed the house.

void DoGameOver (void)
{
	DrawSurface *surface = mainWindow->GetDrawSurface();
	playing = false;
	SetUpFinalScreen();
	ColorRect(surface, mainWindowRect, 244);
	DoGameOverStarAnimation();
	if (!TestHighScore())
		RedrawSplashScreen();
}

//--------------------------------------------------------------  SetUpFinalScreen

// This sets up the game over screen (again, this function is for when…
// the player completes the house).

void SetUpFinalScreen (void)
{
	Rect		tempRect;
	Str255		tempStr, subStr;
	short		count, offset, i, textDown;
	char		wasState;
	DrawSurface	*surface = workSrcMap;

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	
	ColorRect(surface, workSrcRect, 244);
	QSetRect(&tempRect, 0, 0, 640, 460);
	CenterRectInRect(&tempRect, &workSrcRect);
	LoadScaledGraphic(surface, kMilkywayPictID, &tempRect);
	textDown = tempRect.top;
	if (textDown < 0)
		textDown = 0;
	
	PasStringCopy((*thisHouse)->trailer, tempStr);
	
	count = 0;
	do
	{
		GetLineOfText(tempStr, count, subStr);

		PortabilityLayer::RenderedFont *appFont = GetFont(PortabilityLayer::FontPresets::kApplication12Bold);
		offset = ((thisMac.constrainedScreen.right - thisMac.constrainedScreen.left) -
				appFont->MeasurePStr(subStr)) / 2;

		const Point textShadowPos = Point::Create(offset + 1, textDown + 33 + (count * 20));

		surface->DrawString(textShadowPos, subStr, blackColor, appFont);

		const Point textPos = Point::Create(offset, textDown + 32 + (count * 20));
		surface->DrawString(textPos, subStr, whiteColor, appFont);
		count++;
	}
	while (subStr[0] > 0);
	
	CopyRectWorkToBack(&workSrcRect);
	
	for (i = 0; i < 5; i++)		// initialize the falling stars
	{
		pages[i].dest = starSrc[0];
		QOffsetRect(&pages[i].dest, 
				workSrcRect.right + RandomInt(workSrcRect.right / 5) + 
				(workSrcRect.right/ 4) * i, 
				RandomInt(workSrcRect.bottom) - workSrcRect.bottom / 2);
		pages[i].was = pages[i].dest;
		pages[i].frame = RandomInt(6);
	}
}

//--------------------------------------------------------------  DoGameOverStarAnimation

// This handles the falling stars and the flying angel when a player…
// completes a house.

void DoGameOverStarAnimation (void)
{
	#define		kStarFalls	8
	TimeTaggedVOSEvent	theEvent;
	Rect		angelDest;
	long		nextLoop;
	short		which, i, count, pass;
	Boolean		noInteruption;

	short starFallSpeed = kStarFalls;
	const int kStarSpacing = 32;
	const int kAngelSpeed = 2;
	const int kStarsReserved = 5;
	const int kMaxFramesAlive = (kStarSpacing * kStarsReserved + kAngelSpeed - 1) / kAngelSpeed;
	
	angelDest = angelSrcRect;
	QOffsetRect(&angelDest, -96, 0);
	noInteruption = true;
	nextLoop = TickCount() + 2;
	count = 0;
	pass = 0;
	FlushEvents();

	if (workSrcRect.bottom - angelDest.bottom > kMaxFramesAlive * starFallSpeed)
		starFallSpeed = (workSrcRect.bottom - angelDest.bottom + kMaxFramesAlive - 1) / kMaxFramesAlive;
	
	while (noInteruption)
	{
		if ((angelDest.left % kStarSpacing) == 0)		// add a star
		{
			PlayPrioritySound(kMysticSound, kMysticPriority);
			which = angelDest.left / kStarSpacing;
			which = which % kStarsReserved;
			if (which < 0)
				which += kStarsReserved;
			ZeroRectCorner(&pages[which].dest);
			QOffsetRect(&pages[which].dest, angelDest.left, angelDest.bottom);
			if (count < (which + 1))
				count = which + 1;
		}
		
		for (i = 0; i < count; i++)
		{
			pages[i].frame++;
			if (pages[i].frame >= 6)
				pages[i].frame = 0;
			
			CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
					(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
					(BitMap *)*GetGWorldPixMap(workSrcMap), 
					&starSrc[pages[i].frame], 
					&starSrc[pages[i].frame], 
					&pages[i].dest);
			
			pages[i].was = pages[i].dest;
			pages[i].was.top -= starFallSpeed;
			
			AddRectToWorkRectsWhole(&pages[i].was);
			AddRectToBackRects(&pages[i].dest);
			
			if (pages[i].dest.top < workSrcRect.bottom)
				QOffsetRect(&pages[i].dest, 0, starFallSpeed);
		}
		
		if (angelDest.left <= (workSrcRect.right + 2))
		{
			CopyMask((BitMap *)*GetGWorldPixMap(angelSrcMap), 
					(BitMap *)*GetGWorldPixMap(angelMaskMap), 
					(BitMap *)*GetGWorldPixMap(workSrcMap), 
					&angelSrcRect, &angelSrcRect, &angelDest);
			angelDest.left -= kAngelSpeed;
			AddRectToWorkRectsWhole(&angelDest);
			angelDest.left += kAngelSpeed;
			AddRectToBackRects(&angelDest);
			QOffsetRect(&angelDest, kAngelSpeed, 0);
			pass = 0;
		}
		
		CopyRectsQD();
		
		numWork2Main = 0;
		numBack2Work = 0;
		
		do
		{
			const KeyDownStates *theKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

			if ((theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kControl))) || (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kAlt))) || (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kShift))))
				noInteruption = false;

			if (PortabilityLayer::EventQueue::GetInstance()->Dequeue(&theEvent))
			{
				if (theEvent.IsLMouseDownEvent() || theEvent.IsKeyDownEvent())
					noInteruption = false;
			}

			Delay(1, nullptr);
		}
		while (TickCount() < nextLoop);
		nextLoop = TickCount() + 2;
		
		if (pass < 80)
			pass++;
		else
		{
			WaitForInputEvent(5);
			noInteruption = false;
		}
	}
}

//--------------------------------------------------------------  FlagGameOver

// Called to indicate (flag) that a game is over.  Actual game over…
// sequence comes up after a short delay.

void FlagGameOver (void)
{
	gameOver = true;
	countDown = kNumCountDownFrames;
	SetMusicalMode(kPlayWholeScoreMode);
}

//--------------------------------------------------------------  InitDiedGameOver
// This is called when a game is over due to the fact that the player…
// lost their last glider (died), not due to getting through the entire…
// house.  This function initializes the strucures/variables.

void InitDiedGameOver (void)
{
	#define		kPageSpacing		40
	#define		kPageRightOffset	128
	#define		kPageBackUp			128
	short		i;
	PLError_t		theErr;
	
	QSetRect(&pageSrcRect, 0, 0, 25, 32 * 8);
	theErr = CreateOffScreenGWorld(&gameOverSrcMap, &pageSrcRect);
	LoadGraphic(gameOverSrcMap, kLettersPictID);
	
	QSetRect(&pageSrcRect, 0, 0, 32, 32 * kPageFrames);
	theErr = CreateOffScreenGWorld(&pageSrcMap, &pageSrcRect);
	LoadGraphic(pageSrcMap, kPagesPictID);
	
	theErr = CreateOffScreenGWorldCustomDepth(&pageMaskMap, &pageSrcRect, GpPixelFormats::kBW1);
	LoadGraphic(pageMaskMap, kPagesMaskID);
	
	for (i = 0; i < kPageFrames; i++)	// initialize src page rects
	{
		QSetRect(&pageSrc[i], 0, 0, 32, 32);
		QOffsetRect(&pageSrc[i], 0, 32 * i);
	}
	
	for (i = 0; i < 8; i++)				// initialize dest page rects
	{
		QSetRect(&pages[i].dest, 0, 0, 32, 32);
		CenterRectInRect(&pages[i].dest, &thisMac.constrainedScreen);
		QOffsetRect(&pages[i].dest, -thisMac.constrainedScreen.left, -thisMac.constrainedScreen.top);
		if (i < 4)
			QOffsetRect(&pages[i].dest, -kPageSpacing * (4 - i), 0);
		else
			QOffsetRect(&pages[i].dest, kPageSpacing * (i - 3), 0);
		QOffsetRect(&pages[i].dest, (thisMac.constrainedScreen.right - thisMac.constrainedScreen.left) / -2,
				(thisMac.constrainedScreen.right - thisMac.constrainedScreen.left) / -2);
		if (pages[i].dest.left % 2 == 1)
			QOffsetRect(&pages[i].dest, 1, 0);
		pages[i].was = pages[i].dest;
		pages[i].frame = 0;
		pages[i].counter = RandomInt(32);
		pages[i].stuck = false;
	}
	
	for (i = 0; i < 8; i++)
	{
		QSetRect(&lettersSrc[i], 0, 0, 25, 32);
		QOffsetRect(&lettersSrc[i], 0, 32 * i);
	}

	pagesStuck = 0;
	stopPages = ((thisMac.constrainedScreen.bottom - thisMac.constrainedScreen.top) / 2) - 16;
}

//--------------------------------------------------------------  HandlePages

// This handles the pieces of paper that blow across the screen.

void HandlePages (void)
{
	short		i;
	
	for (i = 0; i < 8; i++)
	{
		if ((pages[i].dest.bottom + RandomInt(8)) > stopPages)
		{
			pages[i].frame = 0;
			if (!pages[i].stuck)
			{
				pages[i].dest.right = pages[i].dest.left + 25;
				pages[i].stuck = true;
				pagesStuck++;
			}
		}
		else
		{
			if (pages[i].frame == 0)
			{
				pages[i].counter--;
				if (pages[i].counter <= 0)
					pages[i].frame = 1;
			}
			else if (pages[i].frame == 7)
			{
				pages[i].counter--;
				if (pages[i].counter <= 0)
				{
					pages[i].frame = 8;
					if (RandomInt(2) == 0)
						PlayPrioritySound(kPaper3Sound, kPapersPriority);
					else
						PlayPrioritySound(kPaper4Sound, kPapersPriority);
				}
				else
					QOffsetRect(&pages[i].dest, 10, 10);
			}
			else
			{
				pages[i].frame++;
				switch (pages[i].frame)
				{
					case 5:
					QOffsetRect(&pages[i].dest, 6, 6);
					break;
					
					case 6:
					QOffsetRect(&pages[i].dest, 8, 8);
					break;
					
					case 7:
					QOffsetRect(&pages[i].dest, 8, 8);
					pages[i].counter = RandomInt(4) + 4;
					break;
					
					case 8:
					case 9:
					QOffsetRect(&pages[i].dest, 8, 8);
					break;
					
					case 10:
					QOffsetRect(&pages[i].dest, 6, 6);
					break;
					
					case kPageFrames:
					QOffsetRect(&pages[i].dest, 8, 0);
					pages[i].frame = 0;
					pages[i].counter = RandomInt(8) + 8;
					if (RandomInt(2) == 0)
						PlayPrioritySound(kPaper1Sound, kPapersPriority);
					else
						PlayPrioritySound(kPaper2Sound, kPapersPriority);
					break;
				}
			}
		}
	}
}

//--------------------------------------------------------------  DrawPages

// This function does the drawing for the pieces of paper that blow… 
// across the screen.

void DrawPages (void)
{
	short		i;
	
	for (i = 0; i < 8; i++)
	{
		if (pages[i].stuck)
		{
			CopyBitsConstrained((BitMap *)*GetGWorldPixMap(gameOverSrcMap), 
					(BitMap *)*GetGWorldPixMap(workSrcMap), 
					&lettersSrc[i], &pages[i].dest, 
					srcCopy, &justRoomsRect);
		}
		else
		{
			CopyMask((BitMap *)*GetGWorldPixMap(pageSrcMap), 
					(BitMap *)*GetGWorldPixMap(pageMaskMap), 
					(BitMap *)*GetGWorldPixMap(workSrcMap), 
					&pageSrc[pages[i].frame], 
					&pageSrc[pages[i].frame], 
					&pages[i].dest);
		}
		
		QUnionSimilarRect(&pages[i].dest, &pages[i].was, &pages[i].was);
		AddRectToWorkRects(&pages[i].was);
		AddRectToBackRects(&pages[i].dest);
		
		CopyRectsQD();
		
		numWork2Main = 0;
		numBack2Work = 0;
		
		pages[i].was = pages[i].dest;
	}
}

//--------------------------------------------------------------  DoDiedGameOver

// This is called when a game is over due to the fact that the player…
// lost their last glider (died), not due to getting through the entire…
// house.

void DoDiedGameOver (void)
{
	TimeTaggedVOSEvent	theEvent;
	long				nextLoop;
	Boolean				userAborted;
	
	userAborted = false;
	InitDiedGameOver();
	CopyRectMainToWork(&workSrcRect);
	CopyRectMainToBack(&workSrcRect);
	FlushEvents();
	
	nextLoop = TickCount() + 2;
	while (pagesStuck < 8)
	{
		HandlePages();
		DrawPages();
		do
		{
			const KeyDownStates *theKeys = PortabilityLayer::InputManager::GetInstance()->GetKeys();

			if ((theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kAlt))) || (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kControl))) || (theKeys->IsSet(PL_KEY_EITHER_SPECIAL(kShift))))
			{
				pagesStuck = 8;
				userAborted = true;
			}

			if (PortabilityLayer::EventQueue::GetInstance()->Dequeue(&theEvent))
			{
				if (theEvent.IsLMouseDownEvent() || theEvent.IsKeyDownEvent())
				{
					pagesStuck = 8;
					userAborted = true;
				}
			}

			PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
			Delay(1, nullptr);
		}
		while (TickCount() < nextLoop);
		nextLoop = TickCount() + 2;
	}
	
	DisposeGWorld(pageSrcMap);
	pageSrcMap = nil;
	
	DisposeGWorld(pageMaskMap);
	pageMaskMap = nil;
	
	DisposeGWorld(gameOverSrcMap);
	gameOverSrcMap = nil;
	playing = false;
	
	if (demoGoing)
	{
		if (!userAborted)
			WaitForInputEvent(1);
	}
	else
	{
		if (!userAborted)
			WaitForInputEvent(10);
		TestHighScore();
	}
	RedrawSplashScreen();
}

