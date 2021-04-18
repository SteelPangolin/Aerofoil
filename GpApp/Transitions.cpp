
//============================================================================
//----------------------------------------------------------------------------
//								 Transitions.c
//----------------------------------------------------------------------------
//============================================================================


#include "Externs.h"
#include "Environ.h"
#include "MainWindow.h"
#include "MemoryManager.h"
#include "QDPixMap.h"
#include "PLQDraw.h"
#include "RectUtils.h"
#include "RandomNumberGenerator.h"
#include "PLSysCalls.h"

#include <algorithm>

extern Boolean quickerTransitions;


//==============================================================  Functions
//--------------------------------------------------------------  PourScreenOn

void PourScreenOn (Rect *theRect)
{
	#define		kMaxColumnsWide	96
	#define		kChipHigh		20
	#define		kChipWide		16
	Rect		columnRects[kMaxColumnsWide];
	short		columnProgress[kMaxColumnsWide];
	short		i, colsComplete, colWide, rowTall;
	Boolean		working;
	
	colWide = theRect->right / kChipWide;			// determine # of cols
	rowTall = (theRect->bottom / kChipHigh) + 1;	// determine # of rows
	
	working = true;
	colsComplete = 0;
	for (i = 0; i < colWide; i++)
	{
		columnProgress[i] = 0;
		QSetRect(&columnRects[i], 0, 0, kChipWide, kChipHigh);
		QOffsetRect(&columnRects[i], (i * kChipWide) + theRect->left, theRect->top);
	}

	const int kUnitsPerBlock = 128;

	int unitsCommitted = 0;
	while (working)
	{
		do
		{
			i = RandomInt(colWide);
		}
		while (columnProgress[i] >= rowTall);
		
		if (columnRects[i].left < theRect->left)
			columnRects[i].left = theRect->left;
		if (columnRects[i].top < theRect->top)
			columnRects[i].top = theRect->top;
		if (columnRects[i].right > theRect->right)
			columnRects[i].right = theRect->right;
		if (columnRects[i].bottom > theRect->bottom)
			columnRects[i].bottom = theRect->bottom;
		
		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap), 
				GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()), 
				&columnRects[i], &columnRects[i], srcCopy);
				
		QOffsetRect(&columnRects[i], 0, kChipHigh);
		columnProgress[i]++;
		if (columnProgress[i] >= rowTall)
		{
			colsComplete++;
			if (colsComplete >= colWide)
				working = false;
		}

		unitsCommitted++;

		if (unitsCommitted == kUnitsPerBlock)
		{
			mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
			Delay(1, nullptr);

			unitsCommitted = 0;
		}
	}

	mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
}

//--------------------------------------------------------------  WipeScreenOn

void WipeScreenOn (short direction, Rect *theRect)
{
	if (quickerTransitions)
	{
		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap),
			GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()),
			theRect, theRect, srcCopy);

		mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

		return;
	}

	Rect		wipeRect;
	short		hOffset, vOffset;
	short		i, count;

	const int kWipeTransitionTime = 10;

	const int wipeRectThick = (theRect->Width() + kWipeTransitionTime - 1) / kWipeTransitionTime;
	
	wipeRect = *theRect;
	switch (direction)
	{
		case kAbove:
		wipeRect.bottom = wipeRect.top + wipeRectThick;
		hOffset = 0;
		vOffset = wipeRectThick;
		count = (theRect->bottom - theRect->top + wipeRectThick - 1) / wipeRectThick;
		break;
		
		case kToRight:
		wipeRect.left = wipeRect.right - wipeRectThick;
		hOffset = -wipeRectThick;
		vOffset = 0;
		count = (workSrcRect.right + wipeRectThick - 1) / wipeRectThick;
		break;
		
		case kBelow:
		wipeRect.top = wipeRect.bottom - wipeRectThick;
		hOffset = 0;
		vOffset = -wipeRectThick;
		count = (theRect->bottom - theRect->top + wipeRectThick - 1) / wipeRectThick;
		break;
		
		case kToLeft:
		wipeRect.right = wipeRect.left + wipeRectThick;
		hOffset = wipeRectThick;
		vOffset = 0;
		count = (workSrcRect.right + wipeRectThick - 1) / wipeRectThick;
		break;
	}
	
	for (i = 0; i < count; i++)
	{
		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap), 
				GetPortBitMapForCopyBits(mainWindow->GetDrawSurface()), 
				&wipeRect, &wipeRect, srcCopy);
		
		QOffsetRect(&wipeRect, hOffset, vOffset);
		
		if (wipeRect.top < theRect->top)
			wipeRect.top = theRect->top;
		else if (wipeRect.top > theRect->bottom)
			wipeRect.top = theRect->bottom;
		if (wipeRect.bottom < theRect->top)
			wipeRect.bottom = theRect->top;
		else if (wipeRect.bottom > theRect->bottom)
			wipeRect.bottom = theRect->bottom;

		mainWindow->GetDrawSurface()->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

		PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
		Delay(1, nullptr);
	}
}

//--------------------------------------------------------------  DissolveScreenOn

void DissolveScreenOn(Rect *theRect)
{
	DrawSurface *graf = mainWindow->GetDrawSurface();

	const int kChunkHeight = 15;
	const int kChunkWidth = 20;

	const int rows = (theRect->Height() + kChunkHeight - 1) / kChunkHeight;
	const int cols = (theRect->Width() + kChunkWidth - 1) / kChunkWidth;

	const int numCells = rows * cols;

	const int targetTransitionTime = 30;

	Point *points = static_cast<Point*>(PortabilityLayer::MemoryManager::GetInstance()->Alloc(sizeof(Point) * numCells));

	int rectLeft = theRect->left;
	int rectTop = theRect->top;

	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
			points[col + row * cols] = Point::Create(col * kChunkWidth + rectLeft, row * kChunkHeight + rectTop);
	}

	PortabilityLayer::RandomNumberGenerator *rng = PortabilityLayer::RandomNumberGenerator::GetInstance();

	for (unsigned int shuffleIndex = 0; shuffleIndex < static_cast<unsigned int>(numCells - 1); shuffleIndex++)
	{
		unsigned int shuffleRange = static_cast<unsigned int>(numCells - 1) - shuffleIndex;
		unsigned int shuffleTarget = (rng->GetNextAndAdvance() % shuffleRange) + shuffleIndex;

		if (shuffleTarget != shuffleIndex)
			std::swap(points[shuffleIndex], points[shuffleTarget]);
	}

	const int numCellsAtOnce = numCells / targetTransitionTime;

	const BitMap *srcBitmap = *GetGWorldPixMap(workSrcMap);
	BitMap *destBitmap = GetPortBitMapForCopyBits(graf);

	for (unsigned int firstCell = 0; firstCell < static_cast<unsigned int>(numCells); firstCell += numCellsAtOnce)
	{
		unsigned int lastCell = firstCell + numCellsAtOnce;
		if (lastCell > static_cast<unsigned int>(numCells))
			lastCell = numCells;

		for (unsigned int i = firstCell; i < lastCell; i++)
		{
			const Point &point = points[i];
			const Rect copyRect = Rect::Create(point.v, point.h, point.v + kChunkHeight, point.h + kChunkWidth);

			CopyBits(srcBitmap, destBitmap, &copyRect, &copyRect, srcCopy);
		}

		graf->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

		{
			PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();
			Delay(1, nullptr);
		}
	}

	graf->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);

	PortabilityLayer::MemoryManager::GetInstance()->Release(points);
}

//--------------------------------------------------------------  DumpScreenOn

void DumpScreenOn(Rect *theRect, Boolean fast)
{
	if (quickerTransitions || fast)
	{
		DrawSurface *graf = mainWindow->GetDrawSurface();

		CopyBits((BitMap *)*GetGWorldPixMap(workSrcMap),
			GetPortBitMapForCopyBits(graf),
			theRect, theRect, srcCopy);

		graf->m_port.SetDirty(PortabilityLayer::QDPortDirtyFlag_Contents);
	}
	else
		DissolveScreenOn(theRect);
}

