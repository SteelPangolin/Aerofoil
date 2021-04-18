
//============================================================================
//----------------------------------------------------------------------------
//								   SavedGames.c
//----------------------------------------------------------------------------
//============================================================================

#include "PLDialogs.h"
#include "PLStringCompare.h"
#include "DialogManager.h"
#include "Externs.h"
#include "FileBrowserUI.h"
#include "FileManager.h"
#include "FontFamily.h"
#include "House.h"
#include "GpIOStream.h"
#include "InputManager.h"
#include "MacFileInfo.h"
#include "MemoryManager.h"
#include "PLStandardColors.h"
#include "ResolveCachingColor.h"

#include <assert.h>

#define kSavedGameVersion		0x0200


void SavedGameMismatchError (StringPtr);


gameType	smallGame;

extern	VFileSpec	*theHousesSpecs;
extern	short		numStarsRemaining, thisHouseIndex;
extern	Boolean		twoPlayerGame;


//==============================================================  Functions

static const int kStarsOffset = 180;
static const int kGlidersOffset = 260;
static const int kScoreOffset = 320;

static void FBUI_Save_DrawLabels(DrawSurface *surface, const Point &basePoint)
{
	PortabilityLayer::ResolveCachingColor blackColor(StdColors::Black());
	PortabilityLayer::RenderedFont *rfont = GetFont(PortabilityLayer::FontPresets::kSystem12Bold);

	surface->DrawString(basePoint + Point::Create(kStarsOffset, 0), PSTR("Stars Left"), blackColor, rfont);
	surface->DrawString(basePoint + Point::Create(kGlidersOffset, 0), PSTR("Gliders"), blackColor, rfont);
	surface->DrawString(basePoint + Point::Create(kScoreOffset, 0), PSTR("Score"), blackColor, rfont);
}

static void FBUI_Save_DrawFileDetails(DrawSurface *surface, const Point &basePoint, const Rect &constraintRect, void *fileDetails)
{
	PortabilityLayer::ResolveCachingColor blackColor(StdColors::Black());
	PortabilityLayer::RenderedFont *rfont = GetFont(PortabilityLayer::FontPresets::kSystem12Bold);

	const game2Type *gameData = static_cast<const game2Type*>(fileDetails);

	Str255 numStr;

	NumToString(gameData->wasStarsLeft, numStr);
	surface->DrawString(basePoint + Point::Create(kStarsOffset, 0), numStr, blackColor, rfont);

	NumToString(gameData->numGliders, numStr);
	surface->DrawString(basePoint + Point::Create(kGlidersOffset, 0), numStr, blackColor, rfont);

	NumToString(gameData->score, numStr);
	surface->DrawString(basePoint + Point::Create(kScoreOffset, 0), numStr, blackColor, rfont);
}

static void *FBUI_Save_LoadFileDetails(PortabilityLayer::VirtualDirectory_t dirID, const PLPasStr &filename)
{
	GpIOStream *stream = nullptr;
	if (PortabilityLayer::FileManager::GetInstance()->OpenNonCompositeFile(dirID, filename, ".sav", PortabilityLayer::EFilePermission_Read, GpFileCreationDispositions::kOpenExisting, stream) != PLErrors::kNone)
		return nullptr;

	const size_t kPrefixSize = sizeof(game2Type) - sizeof(savedRoom);

	game2Type *gameData = static_cast<game2Type*>(PortabilityLayer::MemoryManager::GetInstance()->Alloc(kPrefixSize));
	if (!gameData)
	{
		stream->Close();
		return nullptr;
	}

	if (stream->Read(gameData, kPrefixSize) != kPrefixSize)
	{
		PortabilityLayer::MemoryManager::GetInstance()->Release(gameData);
		stream->Close();
		return nullptr;
	}

	stream->Close();

	return gameData;
}

static void FBUI_Save_FreeFileDetails(void *fileDetails)
{
	PortabilityLayer::MemoryManager::GetInstance()->Release(fileDetails);
}

static bool FBUI_Save_FilterFile(PortabilityLayer::VirtualDirectory_t dirID, const PLPasStr &filename)
{
	return true;
}

static PortabilityLayer::FileBrowserUI_DetailsCallbackAPI GetSavedGameDetailsAPI()
{
	PortabilityLayer::FileBrowserUI_DetailsCallbackAPI api;

	api.m_drawLabelsCallback = FBUI_Save_DrawLabels;
	api.m_drawFileDetailsCallback = FBUI_Save_DrawFileDetails;
	api.m_loadFileDetailsCallback = FBUI_Save_LoadFileDetails;
	api.m_freeFileDetailsCallback = FBUI_Save_FreeFileDetails;
	api.m_filterFileCallback = FBUI_Save_FilterFile;

	return api;
}

//--------------------------------------------------------------  SaveGame2

Boolean SaveGame2 (void)
{
	// Bringing up the save file UI can cause key/mouse events to be missed, resulting in state being stuck when this comes back.
	// To avoid that, clear all state here.
	PortabilityLayer::InputManager::GetInstance()->ClearState();

	Str255				gameNameStr;
	size_t				byteCount;
	houseType			*thisHousePtr;
	roomType			*srcRoom;
	savedRoom			*destRoom;
	gamePtr				savedGame;
	short				r, i, numRooms;
	char				wasState;
	GpIOStream	*gameStream = nullptr;

	PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();
	PortabilityLayer::FileManager *fm = PortabilityLayer::FileManager::GetInstance();
	
	FlushEvents();
	
	thisHousePtr = *thisHouse;
	
	numRooms = thisHousePtr->nRooms;
	
	byteCount = sizeof(game2Type) + sizeof(savedRoom) * numRooms;
	savedGame = (gamePtr)mm->Alloc(byteCount);
	if (savedGame == nil)
	{
		YellowAlert(kYellowFailedSaveGame, PLErrors::kOutOfMemory);
		return false;
	}

	memset(savedGame, 0, byteCount);
	
	GetFirstWordOfString(thisHouseName, gameNameStr);
	if (gameNameStr[0] > 23)
		gameNameStr[0] = 23;
	PasStringConcat(gameNameStr, PSTR(" Game"));

	VFileSpec spec;
	spec.m_dir = PortabilityLayer::VirtualDirectories::kUserSaves;
	spec.m_name[0] = 0;

	char savePath[sizeof(spec.m_name) + 1];
	size_t savePathLength = 0;

	if (!fm->PromptSaveFile(spec.m_dir, ".sav", savePath, savePathLength, sizeof(spec.m_name), PLPasStr(gameNameStr), PSTR("Save Game"), false, GetSavedGameDetailsAPI()))
	{
		mm->Release(savedGame);
		return false;
	}

	assert(savePathLength < sizeof(spec.m_name) - 1);

	spec.m_name[0] = static_cast<uint8_t>(savePathLength);
	memcpy(spec.m_name + 1, savePath, savePathLength);
	
	thisHousePtr = *thisHouse;
	
	savedGame->house = theHousesSpecs[thisHouseIndex];
	savedGame->version = kSavedGameVersion;
	savedGame->wasStarsLeft = numStarsRemaining;
	savedGame->timeStamp = thisHousePtr->timeStamp;
	savedGame->where.h = theGlider.dest.left;
	savedGame->where.v = theGlider.dest.top;
	savedGame->score = theScore;
	savedGame->unusedLong = 0L;
	savedGame->unusedLong2 = 0L;
	savedGame->energy = batteryTotal;
	savedGame->bands = bandsTotal;
	savedGame->roomNumber = thisRoomNumber;
	savedGame->gliderState = theGlider.mode;
	savedGame->numGliders = mortals;
	savedGame->foil = foilTotal;
	savedGame->nRooms = numRooms;
	savedGame->facing = theGlider.facing;
	savedGame->showFoil = showFoil;
	
	for (r = 0; r < numRooms; r++)
	{
		destRoom = &(savedGame->savedData[r]);
		srcRoom = &(thisHousePtr->rooms[r]);
		
		destRoom->unusedShort = 0;
		destRoom->unusedByte = 0;
		destRoom->visited = srcRoom->visited;
		for (i = 0; i < kMaxRoomObs; i++)
			destRoom->objects[i] = srcRoom->objects[i];
	}

	PLError_t theErr = fm->OpenNonCompositeFile(spec.m_dir, spec.m_name, ".sav", PortabilityLayer::EFilePermission_Write, GpFileCreationDispositions::kCreateOrOverwrite, gameStream);
	if (CheckFileError(theErr, PSTR("Saved Game")))
	{
		if (gameStream->Write(savedGame, byteCount) != byteCount)
		{
			CheckFileError(PLErrors::kIOError, PSTR("Saved Game"));
		}

		gameStream->Close();
	}

	mm->Release(savedGame);

	return true;
}

//--------------------------------------------------------------  SavedGameMismatchError

void SavedGameMismatchError (StringPtr gameName)
{
	#define		kSavedGameErrorAlert	1044
	short		whoCares;
	
	InitCursor();
	
//	CenterAlert(kSavedGameErrorAlert);
	DialogTextSubstitutions substitutions(gameName, thisHouseName, PSTR(""), PSTR(""));
	
	whoCares = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kSavedGameErrorAlert, &substitutions);
}

//--------------------------------------------------------------  OpenSavedGame

Boolean OpenSavedGame (void)
{
	houseType			*thisHousePtr;
	roomType			*destRoom;
	savedRoom			*srcRoom;
	gamePtr				savedGame;
	short				r, i, gameRefNum;
	char				wasState;

	PortabilityLayer::FileManager *fm = PortabilityLayer::FileManager::GetInstance();
	PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();


	VFileSpec spec;
	spec.m_dir = PortabilityLayer::VirtualDirectories::kUserSaves;
	spec.m_name[0] = 0;

	char savePath[sizeof(spec.m_name) + 1];
	size_t savePathLength = 0;

	if (!fm->PromptOpenFile(spec.m_dir, ".sav", savePath, savePathLength, sizeof(spec.m_name), PSTR("Open Saved Game"), false, GetSavedGameDetailsAPI()))
		return false;

	assert(savePathLength < sizeof(spec.m_name) - 1);

	spec.m_name[0] = static_cast<uint8_t>(savePathLength);
	memcpy(spec.m_name + 1, savePath, savePathLength);

	GpIOStream *gameStream = nullptr;
	PLError_t theErr = fm->OpenNonCompositeFile(spec.m_dir, spec.m_name, ".sav", PortabilityLayer::EFilePermission_Read, GpFileCreationDispositions::kOpenExisting, gameStream);

	if (!CheckFileError(theErr, PSTR("Saved Game")))
		return(false);

	const GpUFilePos_t fileSizeFP = gameStream->Size();
	if (fileSizeFP > SIZE_MAX)
	{
		gameStream->Close();
		return false;
	}

	const size_t byteCount = static_cast<size_t>(fileSizeFP);

	savedGame = (gamePtr)mm->Alloc(byteCount);
	if (savedGame == nil)
	{
		YellowAlert(kYellowFailedSaveGame, PLErrors::kOutOfMemory);
		gameStream->Close();
		return(false);
	}
	
	if (gameStream->Read(savedGame, byteCount) != byteCount)
	{
		CheckFileError(PLErrors::kIOError, PSTR("Saved Game"));
		mm->Release(savedGame);
		gameStream->Close();
		return(false);
	}
	
	thisHousePtr = *thisHouse;
	
	if (!StrCmp::Equal(savedGame->house.m_name, thisHouseName))
	{
		SavedGameMismatchError(savedGame->house.m_name);
		mm->Release(savedGame);
		gameStream->Close();
		return(false);
	}
	else if (thisHousePtr->timeStamp != savedGame->timeStamp)
	{
		YellowAlert(kYellowSavedTimeWrong, 0);
		mm->Release(savedGame);
		gameStream->Close();
		return(false);
	}
	else if (savedGame->version != kSavedGameVersion)
	{
		YellowAlert(kYellowSavedVersWrong, kSavedGameVersion);
		mm->Release(savedGame);
		gameStream->Close();
		return(false);
	}
	else if (savedGame->nRooms != thisHousePtr->nRooms)
	{
		YellowAlert(kYellowSavedRoomsWrong, savedGame->nRooms - thisHousePtr->nRooms);
		mm->Release(savedGame);
		gameStream->Close();
		return(false);
	}
	else
	{
		smallGame.wasStarsLeft = savedGame->wasStarsLeft;
		smallGame.where.h = savedGame->where.h;
		smallGame.where.v = savedGame->where.v;
		smallGame.score = savedGame->score;
		smallGame.unusedLong = savedGame->unusedLong;
		smallGame.unusedLong2 = savedGame->unusedLong2;
		smallGame.energy = savedGame->energy;
		smallGame.bands = savedGame->bands;
		smallGame.roomNumber = savedGame->roomNumber;
		smallGame.gliderState = savedGame->gliderState;
		smallGame.numGliders = savedGame->numGliders;
		smallGame.foil = savedGame->foil;
		smallGame.unusedShort = 0;
		smallGame.facing = savedGame->facing;
		smallGame.showFoil = savedGame->showFoil;
		
		for (r = 0; r < savedGame->nRooms; r++)
		{
			srcRoom = &(savedGame->savedData[r]);
			destRoom = &(thisHousePtr->rooms[r]);
			destRoom->visited = srcRoom->visited;
			for (i = 0; i < kMaxRoomObs; i++)
				destRoom->objects[i] = srcRoom->objects[i];
		}
	}

	mm->Release(savedGame);
	gameStream->Close();

	return (true);
}

//--------------------------------------------------------------  SaveGame

// This is probably about 3 days away from becoming the "old" functionÉ
// for saving games.
#if 0
void SaveGame (Boolean doSave)
{
	houseType		*thisHousePtr;
	UInt32			stamp;
	char			wasState;
	
	if (twoPlayerGame)
		return;
	
	thisHousePtr = *thisHouse;
	
	if (doSave)
	{
		thisHousePtr->savedGame.version = kSavedGameVersion;
		thisHousePtr->savedGame.wasStarsLeft = numStarsRemaining;
		GetDateTime(&stamp);
		thisHousePtr->savedGame.timeStamp = (long)stamp;
		thisHousePtr->savedGame.where.h = theGlider.dest.left;
		thisHousePtr->savedGame.where.v = theGlider.dest.top;
		thisHousePtr->savedGame.score = theScore;
		thisHousePtr->savedGame.unusedLong = 0L;
		thisHousePtr->savedGame.unusedLong2 = 0L;
		thisHousePtr->savedGame.energy = batteryTotal;
		thisHousePtr->savedGame.bands = bandsTotal;
		thisHousePtr->savedGame.roomNumber = thisRoomNumber;
		thisHousePtr->savedGame.gliderState = theGlider.mode;
		thisHousePtr->savedGame.numGliders = mortals;
		thisHousePtr->savedGame.foil = foilTotal;
		thisHousePtr->savedGame.unusedShort = 0;
		thisHousePtr->savedGame.facing = theGlider.facing;
		thisHousePtr->savedGame.showFoil = showFoil;
		
		thisHousePtr->hasGame = true;
	}
	else
	{
		thisHousePtr->hasGame = false;
	}
	
	if (doSave)
	{
		if (!WriteHouse(theMode == kEditMode))
			YellowAlert(kYellowFailedWrite, 0);
	}
}
#endif
