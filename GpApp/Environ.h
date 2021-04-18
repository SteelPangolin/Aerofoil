//============================================================================
//----------------------------------------------------------------------------
//								Environ.h
//----------------------------------------------------------------------------
//============================================================================
#pragma once

#include "PLQDraw.h"


typedef struct
{
	Rect		fullScreen, constrainedScreen, gray;
	long		dirID;
	short		isDepth;
	short		numScreens;
	short		vRefNum;
	Boolean		can1Bit;
	Boolean		can4Bit;
	Boolean		can8Bit;
	Boolean		wasColorOrGray;
	Boolean		hasWNE;
	Boolean		hasSystem7;
	Boolean		hasColor;
	Boolean		hasGestalt;
	Boolean		canSwitch;
	Boolean		canColor;
	Boolean		hasSM3;
	Boolean		hasQT;
	Boolean		hasDrag;
	Boolean		isResolutionDirty;
	Boolean		isTouchscreen;
	Boolean		isMouseTouchscreen;
} macEnviron;


extern macEnviron	thisMac;

void FlushResolutionChange(void);
