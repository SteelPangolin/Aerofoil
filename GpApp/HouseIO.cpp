
//============================================================================
//----------------------------------------------------------------------------
//									HouseIO.c
//----------------------------------------------------------------------------
//============================================================================

#include "BitmapImage.h"
#include "DialogManager.h"
#include "Externs.h"
#include "Environ.h"
#include "FileManager.h"
#include "GpIOStream.h"
#include "House.h"
#include "IGpLogDriver.h"
#include "IGpSystemServices.h"
#include "ObjectEdit.h"
#include "ResourceManager.h"

#include "PLDialogs.h"
#include "PLDrivers.h"
#include "PLMovies.h"
#include "PLResources.h"
#include "PLStringCompare.h"
#include "PLPasStr.h"

#define kSaveChangesAlert		1002
#define kSaveChanges			1
#define kDiscardChanges			2


void OpenHouseMovie (void);
void CloseHouseMovie (void);


AnimationPlayer		theMovie;
Rect		movieRect;
PortabilityLayer::IResourceArchive	*houseResFork;
short		wasHouseVersion;
Boolean		houseOpen, fileDirty;
Boolean		changeLockStateOfHouse, saveHouseLocked, houseIsReadOnly;
Boolean		hasMovie, tvInRoom;
PortabilityLayer::CompositeFile *houseCFile;


extern	VFileSpec	*theHousesSpecs;
extern	short		thisHouseIndex, tvWithMovieNumber;
extern	short		numberRooms, housesFound;
extern	Boolean		noRoomAtAll, quitting, wardBitSet;
extern	Boolean		phoneBitSet, bannerStarCountOn;


//==============================================================  Functions

//--------------------------------------------------------------  OpenHouseMovie

void OpenHouseMovie (void)
{
#ifdef COMPILEQT
	VFileSpec	theSpec;
	Handle		spaceSaver;
	short		movieRefNum;
	Boolean		dataRefWasChanged;
	
	if (thisMac.hasQT)
	{
		theSpec = theHousesSpecs[thisHouseIndex];
		PasStringConcat(theSpec.m_name, PSTR(".mov"));

		AnimationPackage *anim = AnimationPackage::Create();
		if (!anim)
			return;

		PLError_t theErr = anim->Load(theSpec.m_dir, theSpec.m_name);

		if (theErr != PLErrors::kNone)
		{
			anim->Destroy();

			if (theErr != PLErrors::kFileNotFound)
				YellowAlert(kYellowQTMovieNotLoaded, theErr);

			return;
		}

		movieRect = (*anim->GetFrame(0))->GetRect();
		
		hasMovie = true;
		theMovie.SetPackage(anim);

		AnimationManager::GetInstance()->RegisterPlayer(&theMovie);
	}
#endif
}

//--------------------------------------------------------------  CloseHouseMovie

void CloseHouseMovie (void)
{
#ifdef COMPILEQT
	PLError_t		theErr;
	
	if ((thisMac.hasQT) && (hasMovie))
	{
		AnimationManager::GetInstance()->RemovePlayer(&theMovie);

		theMovie.m_animPackage->Destroy();
		theMovie.m_animPackage = nullptr;
	}
#endif
	hasMovie = false;
}

//--------------------------------------------------------------  OpenHouse
// Opens a house (whatever current selection is).  Returns true if all went well.

Boolean OpenHouse (Boolean read)
{
	PLError_t		theErr;
	
	if (houseOpen)
	{
		if (!CloseHouse())
			return(false);
	}
	if ((housesFound < 1) || (thisHouseIndex == -1))
		return(false);
	
	#ifdef COMPILEDEMO
	if (!StrCmp::EqualCaseInsensitive(theHousesSpecs[thisHouseIndex].name, "\pDemo House"))
		return (false);
	#endif

	houseCFile = PortabilityLayer::FileManager::GetInstance()->OpenCompositeFile(theHousesSpecs[thisHouseIndex].m_dir, theHousesSpecs[thisHouseIndex].m_name);
	if (!houseCFile)
		return (false);

	houseIsReadOnly = houseCFile->IsDataReadOnly();

	GpIOStream *houseStream = nil;
	theErr = houseCFile->OpenData(PortabilityLayer::EFilePermission_Any, GpFileCreationDispositions::kCreateOrOpen, houseStream);
	if (!CheckFileError(theErr, thisHouseName))
	{
		houseCFile->Close();
		houseCFile = nil;
		return (false);
	}
	
	houseOpen = true;
	OpenHouseResFork();

	if (hasMovie)
		CloseHouseMovie();

	tvInRoom = false;
	tvWithMovieNumber = -1;
	OpenHouseMovie();

	if (read)
	{
		Boolean readOK = ReadHouse(houseStream);
		houseStream->Close();

		return readOK;
	}

	houseStream->Close();
	
	return (true);
}

//--------------------------------------------------------------  OpenSpecificHouse
// Opens the specific house passed in.

#ifndef COMPILEDEMO
Boolean OpenSpecificHouse (const VFileSpec &specs)
{
	short		i;
	Boolean		itOpened;
	
	if ((housesFound < 1) || (thisHouseIndex == -1))
		return (false);
	
	itOpened = true;
	
	for (i = 0; i < housesFound; i++)
	{
		if ((theHousesSpecs[i].m_dir == specs.m_dir) &&
				(StrCmp::EqualCaseInsensitive(theHousesSpecs[i].m_name, specs.m_name)))
		{
			thisHouseIndex = i;
			PasStringCopy(theHousesSpecs[thisHouseIndex].m_name, thisHouseName);
			if (!OpenHouse(true))
				itOpened = false;
			break;
		}
	}
	
	return (itOpened);
}
#endif


//--------------------------------------------------------------  ReadHouse
// With a house open, this function reads in the actual bits of dataÉ
// into memory.

void ByteSwapPoint(Point *point)
{
	PortabilityLayer::ByteSwap::BigInt16(point->h);
	PortabilityLayer::ByteSwap::BigInt16(point->v);
}

void ByteSwapRect(Rect *rect)
{
	PortabilityLayer::ByteSwap::BigInt16(rect->top);
	PortabilityLayer::ByteSwap::BigInt16(rect->left);
	PortabilityLayer::ByteSwap::BigInt16(rect->bottom);
	PortabilityLayer::ByteSwap::BigInt16(rect->right);
}

template<size_t TSize>
void SanitizePascalStr(uint8_t(&chars)[TSize])
{
	const size_t maxLength = TSize - 1;
	size_t strLength = chars[0];
	if (strLength > maxLength)
	{
		strLength = maxLength;
		chars[0] = static_cast<uint8_t>(maxLength);
	}

	for (size_t i = 1 + strLength; i < TSize; i++)
		chars[i] = 0;
}

void ByteSwapScores(scoresType *scores)
{
	SanitizePascalStr(scores->banner);

	for (int i = 0; i < kMaxScores; i++)
		SanitizePascalStr(scores->names[i]);

	for (int i = 0; i < kMaxScores; i++)
		PortabilityLayer::ByteSwap::BigInt32(scores->scores[i]);

	for (int i = 0; i < kMaxScores; i++)
		PortabilityLayer::ByteSwap::BigUInt32(scores->timeStamps[i]);

	for (int i = 0; i < kMaxScores; i++)
		PortabilityLayer::ByteSwap::BigInt16(scores->levels[i]);
}

void ByteSwapSavedGame(gameType *game)
{
	PortabilityLayer::ByteSwap::BigInt16(game->version);
	PortabilityLayer::ByteSwap::BigInt16(game->wasStarsLeft);
	PortabilityLayer::ByteSwap::BigUInt32(game->timeStamp);
	ByteSwapPoint(&game->where);
	PortabilityLayer::ByteSwap::BigInt32(game->score);
	PortabilityLayer::ByteSwap::BigInt32(game->unusedLong);
	PortabilityLayer::ByteSwap::BigInt32(game->unusedLong2);
	PortabilityLayer::ByteSwap::BigInt16(game->energy);
	PortabilityLayer::ByteSwap::BigInt16(game->bands);

	PortabilityLayer::ByteSwap::BigInt16(game->roomNumber);
	PortabilityLayer::ByteSwap::BigInt16(game->gliderState);
	PortabilityLayer::ByteSwap::BigInt16(game->numGliders);
	PortabilityLayer::ByteSwap::BigInt16(game->foil);
	PortabilityLayer::ByteSwap::BigInt16(game->unusedShort);
}

void ByteSwapBlower(blowerType *blower)
{
	ByteSwapPoint(&blower->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(blower->distance);
}

void ByteSwapFurniture(furnitureType *furniture)
{
	ByteSwapRect(&furniture->bounds);
	PortabilityLayer::ByteSwap::BigInt16(furniture->pict);
}

void ByteSwapBonus(bonusType *bonus)
{
	ByteSwapPoint(&bonus->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(bonus->length);
	PortabilityLayer::ByteSwap::BigInt16(bonus->points);
}

void ByteSwapTransport(transportType *transport)
{
	ByteSwapPoint(&transport->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(transport->tall);
	PortabilityLayer::ByteSwap::BigInt16(transport->where);
}

void ByteSwapSwitch(switchType *sw)
{
	ByteSwapPoint(&sw->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(sw->delay);
	PortabilityLayer::ByteSwap::BigInt16(sw->where);
}

void ByteSwapLight(lightType *light)
{
	ByteSwapPoint(&light->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(light->length);
}

void ByteSwapAppliance(applianceType *appliance)
{
	ByteSwapPoint(&appliance->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(appliance->height);
}

void ByteSwapEnemy(enemyType *enemy)
{
	ByteSwapPoint(&enemy->topLeft);
	PortabilityLayer::ByteSwap::BigInt16(enemy->length);
}

void ByteSwapClutter(clutterType *clutter)
{
	ByteSwapRect(&clutter->bounds);
	PortabilityLayer::ByteSwap::BigInt16(clutter->pict);
}

void ByteSwapObject(objectType *obj, bool isSwappedAfter)
{
	int16_t objWhat = 0;

	if (isSwappedAfter)
		objWhat = obj->what;

	PortabilityLayer::ByteSwap::BigInt16(obj->what);

	if (!isSwappedAfter)
		objWhat = obj->what;

	switch (objWhat)
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
		ByteSwapBlower(&obj->data.a);
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
		ByteSwapFurniture(&obj->data.b);
		break;

	case kRedClock:
	case kBlueClock:
	case kYellowClock:
	case kCuckoo:
	case kPaper:
	case kBattery:
	case kBands:
	case kGreaseRt:
	case kGreaseLf:
	case kFoil:
	case kInvisBonus:
	case kStar:
	case kSparkle:
	case kHelium:
	case kSlider:
		ByteSwapBonus(&obj->data.c);
		break;

	case kUpStairs:
	case kDownStairs:
	case kMailboxLf:
	case kMailboxRt:
	case kFloorTrans:
	case kCeilingTrans:
	case kDoorInLf:
	case kDoorInRt:
	case kDoorExRt:
	case kDoorExLf:
	case kWindowInLf:
	case kWindowInRt:
	case kWindowExRt:
	case kWindowExLf:
	case kInvisTrans:
	case kDeluxeTrans:
		ByteSwapTransport(&obj->data.d);
		break;

	case kLightSwitch:
	case kMachineSwitch:
	case kThermostat:
	case kPowerSwitch:
	case kKnifeSwitch:
	case kInvisSwitch:
	case kTrigger:
	case kLgTrigger:
	case kSoundTrigger:
		ByteSwapSwitch(&obj->data.e);
		break;

	case kCeilingLight:
	case kLightBulb:
	case kTableLamp:
	case kHipLamp:
	case kDecoLamp:
	case kFlourescent:
	case kTrackLight:
	case kInvisLight:
		ByteSwapLight(&obj->data.f);
		break;

	case kShredder:
	case kToaster:
	case kMacPlus:
	case kGuitar:
	case kTV:
	case kCoffee:
	case kOutlet:
	case kVCR:
	case kStereo:
	case kMicrowave:
	case kCinderBlock:
	case kFlowerBox:
	case kCDs:
	case kCustomPict:
		ByteSwapAppliance(&obj->data.g);
		break;

	case kBalloon:
	case kCopterLf:
	case kCopterRt:
	case kDartLf:
	case kDartRt:
	case kBall:
	case kDrip:
	case kFish:
	case kCobweb:
		ByteSwapEnemy(&obj->data.h);
		break;

	case kOzma:
	case kMirror:
	case kMousehole:
	case kFireplace:
	case kFlower:
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
		ByteSwapClutter(&obj->data.i);
		break;
	default:
		break;
	};
}

void ByteSwapRoom(roomType *room, bool isSwappedAfter)
{
	SanitizePascalStr(room->name);

	PortabilityLayer::ByteSwap::BigInt16(room->bounds);

	PortabilityLayer::ByteSwap::BigInt16(room->background);

	for (int i = 0; i < kNumTiles; i++)
		PortabilityLayer::ByteSwap::BigInt16(room->tiles[i]);

	PortabilityLayer::ByteSwap::BigInt16(room->floor);
	PortabilityLayer::ByteSwap::BigInt16(room->suite);
	PortabilityLayer::ByteSwap::BigInt16(room->openings);
	PortabilityLayer::ByteSwap::BigInt16(room->numObjects);
	for (int i = 0; i < kMaxRoomObs; i++)
		ByteSwapObject(room->objects + i, isSwappedAfter);
}

bool ByteSwapHouse(housePtr house, size_t sizeInBytes, bool isSwappedAfter)
{
	size_t nRooms = 0;

	if (isSwappedAfter)
		nRooms = house->nRooms;

	PortabilityLayer::ByteSwap::BigInt16(house->version);
	PortabilityLayer::ByteSwap::BigInt16(house->unusedShort);
	PortabilityLayer::ByteSwap::BigInt32(house->timeStamp);
	PortabilityLayer::ByteSwap::BigInt32(house->flags);
	ByteSwapPoint(&house->initial);
	SanitizePascalStr(house->banner);
	SanitizePascalStr(house->trailer);
	ByteSwapScores(&house->highScores);
	ByteSwapSavedGame(&house->savedGame);
	PortabilityLayer::ByteSwap::BigInt16(house->firstRoom);
	PortabilityLayer::ByteSwap::BigInt16(house->nRooms);

	if (!isSwappedAfter)
		nRooms = house->nRooms;

	const size_t roomDataSize = sizeInBytes - houseType::kBinaryDataSize;
	if (nRooms < 0 || roomDataSize / sizeof(roomType) < nRooms)
		return false;

	for (size_t i = 0; i < nRooms; i++)
		ByteSwapRoom(house->rooms + i, isSwappedAfter);

	house->padding = 0;

	return true;
}

Boolean ReadHouse (GpIOStream *houseStream)
{
	long		byteCount;
	PLError_t		theErr;
	short		whichRoom;

	// There should be no padding remaining the house type
	GP_STATIC_ASSERT(sizeof(houseType) - sizeof(roomType) == houseType::kBinaryDataSize + 2);
	
	if (!houseOpen)
	{
		YellowAlert(kYellowUnaccounted, 2);
		return (false);
	}

	byteCount = houseStream->Size();
	
	#ifdef COMPILEDEMO
	if (byteCount != 16526L)
		return (false);
	#endif
	
	if (thisHouse != nil)
		thisHouse.Dispose();

	// GP: Correct for padding
	const size_t alignmentPadding = sizeof(houseType) - sizeof(roomType) - houseType::kBinaryDataSize;
	
	thisHouse = NewHandle(byteCount + alignmentPadding).StaticCast<houseType>();
	if (thisHouse == nil)
	{
		YellowAlert(kYellowNoMemory, 10);
		return(false);
	}
	
	if (!houseStream->SeekStart(0))
	{
		CheckFileError(PLErrors::kIOError, thisHouseName);
		return(false);
	}
	
	const size_t readByteCount = houseStream->Read(*thisHouse, byteCount);
	if (readByteCount != byteCount || readByteCount < houseType::kBinaryDataSize)
	{
		CheckFileError(PLErrors::kIOError, thisHouseName);
		return(false);
	}

	if (alignmentPadding != 0)
	{
		// GP: Correct for padding
		const size_t roomDataSize = byteCount - houseType::kBinaryDataSize;

		uint8_t *houseDataBytes = reinterpret_cast<uint8_t*>(*thisHouse);
		memmove((*thisHouse)->rooms, houseDataBytes + houseType::kBinaryDataSize, roomDataSize);
	}

	ByteSwapHouse(*thisHouse, static_cast<size_t>(byteCount), false);
	
	numberRooms = (*thisHouse)->nRooms;
	#ifdef COMPILEDEMO
	if (numberRooms != 45)
		return (false);
	#endif
	if ((numberRooms < 1) || (byteCount == 0L))
	{
		numberRooms = 0;
		noRoomAtAll = true;
		YellowAlert(kYellowNoRooms, 0);
	}
	
	wasHouseVersion = (*thisHouse)->version;
	if (wasHouseVersion >= kNewHouseVersion)
	{
		YellowAlert(kYellowNewerVersion, 0);
		return(false);
	}
	
	houseUnlocked = (((*thisHouse)->timeStamp & 0x00000001) == 0);
	#ifdef COMPILEDEMO
	if (houseUnlocked)
		return (false);
	#endif
	changeLockStateOfHouse = false;
	saveHouseLocked = false;
	
	whichRoom = (*thisHouse)->firstRoom;
	#ifdef COMPILEDEMO
	if (whichRoom != 0)
		return (false);
	#endif
	
	wardBitSet = (((*thisHouse)->flags & 0x00000001) == 0x00000001);
	phoneBitSet = (((*thisHouse)->flags & 0x00000002) == 0x00000002);
	bannerStarCountOn = (((*thisHouse)->flags & 0x00000004) == 0x00000000);
	
	noRoomAtAll = (RealRoomNumberCount() == 0);
	thisRoomNumber = -1;
	previousRoom = -1;
	if (!noRoomAtAll)
		CopyRoomToThisRoom(whichRoom);
	
	if (houseIsReadOnly)
	{
		houseUnlocked = false;
		if (ReadScoresFromDisk())
		{
		}
	}
	
	objActive = kNoObjectSelected;
	ReflectCurrentRoom(true);
	fileDirty = false;
	UpdateMenus(false);
	
	return (true);
}

//--------------------------------------------------------------  WriteHouse
// This function writes out the house data to disk.

Boolean WriteHouse (Boolean checkIt)
{
	UInt32			timeStamp;
	long			byteCount;
	PLError_t			theErr;

	if ((housesFound < 1) || (thisHouseIndex == -1))
		return(false);

	if (!houseOpen)
	{
		YellowAlert(kYellowUnaccounted, 4);
		return (false);
	}

	if (!houseCFile)
	{
		YellowAlert(kYellowUnaccounted, 4);
		return (false);
	}

	GpIOStream *houseStream = nil;
	theErr = houseCFile->OpenData(PortabilityLayer::EFilePermission_Write, GpFileCreationDispositions::kCreateOrOpen, houseStream);
	if (theErr != PLErrors::kNone)
		return (false);

	CopyThisRoomToRoom();
	
	if (checkIt)
		CheckHouseForProblems();
	
	byteCount = GetHandleSize(thisHouse.StaticCast<void>());
	
	if (fileDirty)
	{
		int64_t currentTime = PLDrivers::GetSystemServices()->GetTime();
		if (currentTime > 0x7fffffff)
			currentTime = 0x7fffffff;

		if (changeLockStateOfHouse)
			houseUnlocked = !saveHouseLocked;
		
		if (houseUnlocked)								// house unlocked
			timeStamp &= 0x7FFFFFFE;
		else
			timeStamp |= 0x00000001;
		(*thisHouse)->timeStamp = (long)timeStamp;
		(*thisHouse)->version = wasHouseVersion;
	}

	long headerSize = houseType::kBinaryDataSize;
	long roomsSize = sizeof(roomType) * (*thisHouse)->nRooms;

	ByteSwapHouse(*thisHouse, static_cast<size_t>(byteCount), true);

	if (houseStream->Write(*thisHouse, headerSize) != headerSize)
	{
		CheckFileError(PLErrors::kIOError, thisHouseName);
		ByteSwapHouse(*thisHouse, static_cast<size_t>(byteCount), false);
		houseStream->Close();
		return(false);
	}

	if (houseStream->Write((*thisHouse)->rooms, roomsSize) != roomsSize)
	{
		CheckFileError(PLErrors::kIOError, thisHouseName);
		ByteSwapHouse(*thisHouse, static_cast<size_t>(byteCount), false);
		houseStream->Close();
		return(false);
	}

	ByteSwapHouse(*thisHouse, static_cast<size_t>(byteCount), false);

	houseStream->Close();
	
	if (changeLockStateOfHouse)
	{
		changeLockStateOfHouse = false;
		ReflectCurrentRoom(true);
	}
	
	fileDirty = false;
	UpdateMenus(false);
	return (true);
}

//--------------------------------------------------------------  CloseHouse
// This function closes the current house that is open.

Boolean CloseHouse (void)
{
	PLError_t		theErr;
	
	if (!houseOpen)
		return (true);
	
	if (fileDirty)
	{
#ifndef COMPILEDEMO
		if (!QuerySaveChanges())	// false signifies user canceled
			return(false);
#endif
	}
	
	CloseHouseResFork();
	CloseHouseMovie();

	if (houseCFile)
	{
		houseCFile->Close();
		houseCFile = nil;
	}
	
	houseOpen = false;
	
	return (true);
}

//--------------------------------------------------------------  OpenHouseResFork
// Opens the resource fork of the current house that is open.

void OpenHouseResFork (void)
{
	PortabilityLayer::ResourceManager *rm = PortabilityLayer::ResourceManager::GetInstance();
	if (houseResFork == nullptr)
	{
		houseResFork = rm->LoadResFile(houseCFile);
		if (!houseResFork)
			YellowAlert(kYellowFailedResOpen, PLErrors::kResourceError);
	}
}

//--------------------------------------------------------------  CloseHouseResFork
// Closes the resource fork of the current house that is open.

void CloseHouseResFork (void)
{
	if (houseResFork)
	{
		PortabilityLayer::ResourceManager *rm = PortabilityLayer::ResourceManager::GetInstance();

		houseResFork->Destroy();
		houseResFork = nullptr;
	}
}

//--------------------------------------------------------------  QuerySaveChanges
// If changes were made, this function will present the user with aÉ
// dialog asking them if they would like to save the changes.

#ifndef COMPILEDEMO
Boolean QuerySaveChanges (void)
{
	short		hitWhat;
	Boolean		whoCares;
	
	if (!fileDirty)
		return(true);
	
	InitCursor();
//	CenterAlert(kSaveChangesAlert);
	DialogTextSubstitutions substitutions(thisHouseName);
	hitWhat = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kSaveChangesAlert, &substitutions);
	if (hitWhat == kSaveChanges)
	{
		if (wasHouseVersion < kHouseVersion)
			ConvertHouseVer1To2();
		wasHouseVersion = kHouseVersion;
		if (WriteHouse(true))
			return (true);
		else
			return (false);
	}
	else if (hitWhat == kDiscardChanges)
	{
		fileDirty = false;
		if (!quitting)
		{
			whoCares = CloseHouse();
			OpenHouse(true);
		}
		UpdateMenus(false);
		return (true);
	}
	else
		return (false);
}
#endif

//--------------------------------------------------------------  YellowAlert
// This is a dialog used to present an error code and explanationÉ
// to the user when a non-lethal error has occurred.  Ideally, ofÉ
// course, this never is called.

void YellowAlert (short whichAlert, short identifier)
{
	IGpLogDriver *logger = PLDrivers::GetLogDriver();

	if (logger)
		logger->Printf(IGpLogDriver::Category_Warning, "Yellow alert %i identifier %i", static_cast<int>(whichAlert), static_cast<int>(identifier));

	#define		kYellowAlert	1006
	Str255		errStr, errNumStr;
	short		whoCares;
	
	InitCursor();
	
	GetIndString(errStr, kYellowAlert, whichAlert);
	NumToString((long)identifier, errNumStr);
	
//	CenterAlert(kYellowAlert);
	DialogTextSubstitutions substitutions(errStr, errNumStr);
	
	whoCares = PortabilityLayer::DialogManager::GetInstance()->DisplayAlert(kYellowAlert, &substitutions);
}

//--------------------------------------------------------------  LoadHousePicture

THandle<void> LoadHouseResource(const PortabilityLayer::ResTypeID &resTypeID, int16_t resID)
{
	THandle<void> hdl = houseResFork->LoadResource(resTypeID, resID);
	if (hdl != nullptr)
		return hdl;

	return PortabilityLayer::ResourceManager::GetInstance()->GetAppResource(resTypeID, resID);
}
