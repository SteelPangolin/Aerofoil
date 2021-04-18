cmake_minimum_required(VERSION 3.10)

project (Aerofoil)

message(${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_definitions(-DGP_DEBUG_CONFIG=0)

add_library(stb STATIC
	stb/stb_image_write.c
	)

add_library(zlib STATIC
	zlib/adler32.c
	zlib/crc32.c
	zlib/deflate.c
	zlib/inffast.c
	zlib/inflate.c
	zlib/inftrees.c
	zlib/trees.c
	zlib/zutil.c
	)

add_library(MacRomanConversion STATIC
	MacRomanConversion/MacRomanConversion.cpp
	)

add_library(PortabilityLayer STATIC
	PortabilityLayer/AntiAliasTable.cpp
	PortabilityLayer/AppEventHandler.cpp
	PortabilityLayer/BinHex4.cpp
	PortabilityLayer/BitmapImage.cpp
	PortabilityLayer/ByteSwap.cpp
	PortabilityLayer/CFileStream.cpp
	PortabilityLayer/DeflateCodec.cpp
	PortabilityLayer/DialogManager.cpp
	PortabilityLayer/DisplayDeviceManager.cpp
	PortabilityLayer/EllipsePlotter.cpp
	PortabilityLayer/FileBrowserUI.cpp
	PortabilityLayer/FileManager.cpp
	PortabilityLayer/FileSectionStream.cpp
	PortabilityLayer/FontFamily.cpp
	PortabilityLayer/FontManager.cpp
	PortabilityLayer/FontRenderer.cpp
	PortabilityLayer/GPArchive.cpp
	PortabilityLayer/HostSuspendHook.cpp
	PortabilityLayer/IconLoader.cpp
	PortabilityLayer/InflateStream.cpp
	PortabilityLayer/InputManager.cpp
	PortabilityLayer/LinePlotter.cpp
	PortabilityLayer/MacBinary2.cpp
	PortabilityLayer/MacFileInfo.cpp
	PortabilityLayer/MacFileMem.cpp
	PortabilityLayer/MemoryManager.cpp
	PortabilityLayer/MemReaderStream.cpp
	PortabilityLayer/MenuManager.cpp
	PortabilityLayer/MMBlock.cpp
	PortabilityLayer/MMHandleBlock.cpp
	PortabilityLayer/PLApplication.cpp
	PortabilityLayer/PLButtonWidget.cpp
	PortabilityLayer/PLControlDefinitions.cpp
	PortabilityLayer/PLCore.cpp
	PortabilityLayer/PLCTabReducer.cpp
	PortabilityLayer/PLDialogs.cpp
	PortabilityLayer/PLDrivers.cpp
	PortabilityLayer/PLEditboxWidget.cpp
	PortabilityLayer/PLEventQueue.cpp
	PortabilityLayer/PLHacks.cpp
	PortabilityLayer/PLHandle.cpp
	PortabilityLayer/PLIconWidget.cpp
	PortabilityLayer/PLImageWidget.cpp
	PortabilityLayer/PLInvisibleWidget.cpp
	PortabilityLayer/PLKeyEncoding.cpp
	PortabilityLayer/PLLabelWidget.cpp
	PortabilityLayer/PLMenus.cpp
	PortabilityLayer/PLMovies.cpp
	PortabilityLayer/PLNumberFormatting.cpp
	PortabilityLayer/PLPopupMenuWidget.cpp
	PortabilityLayer/PLQDOffscreen.cpp
	PortabilityLayer/PLQDraw.cpp
	PortabilityLayer/PLResourceManager.cpp
	PortabilityLayer/PLResources.cpp
	PortabilityLayer/PLScrollBarWidget.cpp
	PortabilityLayer/PLSound.cpp
	PortabilityLayer/PLStandardColors.cpp
	PortabilityLayer/PLStringCompare.cpp
	PortabilityLayer/PLSysCalls.cpp
	PortabilityLayer/PLTimeTaggedVOSEvent.cpp
	PortabilityLayer/PLWidgets.cpp
	PortabilityLayer/QDGraf.cpp
	PortabilityLayer/QDManager.cpp
	PortabilityLayer/QDPictDecoder.cpp
	PortabilityLayer/QDPictEmitContext.cpp
	PortabilityLayer/QDPictHeader.cpp
	PortabilityLayer/QDPixMap.cpp
	PortabilityLayer/QDPort.cpp
	PortabilityLayer/QDStandardPalette.cpp
	PortabilityLayer/RandomNumberGenerator.cpp
	PortabilityLayer/ResolveCachingColor.cpp
	PortabilityLayer/ResourceCompiledRef.cpp
	PortabilityLayer/ResourceFile.cpp
	PortabilityLayer/ScanlineMask.cpp
	PortabilityLayer/ScanlineMaskBuilder.cpp
	PortabilityLayer/ScanlineMaskConverter.cpp
	PortabilityLayer/ScanlineMaskIterator.cpp
	PortabilityLayer/SimpleGraphic.cpp
	PortabilityLayer/TextPlacer.cpp
	PortabilityLayer/UTF8.cpp
	PortabilityLayer/UTF16.cpp
	PortabilityLayer/WindowDef.cpp
	PortabilityLayer/WindowManager.cpp
	PortabilityLayer/WorkerThread.cpp
	PortabilityLayer/XModemCRC.cpp
	PortabilityLayer/ZipFileProxy.cpp
	)

target_include_directories(PortabilityLayer PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GpCommon>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PortabilityLayer>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/zlib>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/MacRomanConversion>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb>
	)

target_compile_options(PortabilityLayer PRIVATE -Wno-multichar)

target_link_libraries(PortabilityLayer zlib MacRomanConversion stb)


add_library(GpShell STATIC
	GpShell/GpAppEnvironment.cpp
	GpShell/GpAudioDriverFactory.cpp
	GpShell/GpDisplayDriverFactory.cpp
	GpShell/GpFontHandlerFactory.cpp
	GpShell/GpGlobalConfig.cpp
	GpShell/GpInputDriverFactory.cpp
	GpShell/GpMain.cpp
	GpShell/GpVOSEventQueue.cpp
	)

target_include_directories(GpShell PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GpCommon>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PortabilityLayer>
	)

add_library(GpApp STATIC
	GpApp/About.cpp
	GpApp/AnimCursor.cpp
	GpApp/AppleEvents.cpp
	GpApp/Banner.cpp
	GpApp/ColorUtils.cpp
	GpApp/Coordinates.cpp
	GpApp/DialogUtils.cpp
	GpApp/DynamicMaps.cpp
	GpApp/Dynamics.cpp
	GpApp/Dynamics2.cpp
	GpApp/Dynamics3.cpp
	GpApp/Environ.cpp
	GpApp/Events.cpp
	GpApp/FileError.cpp
	GpApp/GameOver.cpp
	GpApp/GpAppInterface.cpp
	GpApp/Grease.cpp
	GpApp/HighScores.cpp
	GpApp/House.cpp
	GpApp/HouseInfo.cpp
	GpApp/HouseIO.cpp
	GpApp/HouseLegal.cpp
	GpApp/Input.cpp
	GpApp/Interactions.cpp
	GpApp/InterfaceInit.cpp
	GpApp/Link.cpp
	GpApp/Main.cpp
	GpApp/MainMenuUI.cpp
	GpApp/MainWindow.cpp
	GpApp/Map.cpp
	GpApp/Marquee.cpp
	GpApp/Menu.cpp
	GpApp/Modes.cpp
	GpApp/Music.cpp
	GpApp/ObjectAdd.cpp
	GpApp/ObjectDraw.cpp
	GpApp/ObjectDraw2.cpp
	GpApp/ObjectDrawAll.cpp
	GpApp/ObjectEdit.cpp
	GpApp/ObjectInfo.cpp
	GpApp/ObjectRects.cpp
	GpApp/Objects.cpp
	GpApp/Play.cpp
	GpApp/Player.cpp
	GpApp/Prefs.cpp
	GpApp/RectUtils.cpp
	GpApp/Render.cpp
	GpApp/Room.cpp
	GpApp/RoomGraphics.cpp
	GpApp/RoomInfo.cpp
	GpApp/RubberBands.cpp
	GpApp/SavedGames.cpp
	GpApp/Scoreboard.cpp
	GpApp/Scrap.cpp
	GpApp/SelectHouse.cpp
	GpApp/Settings.cpp
	GpApp/Sound.cpp
	GpApp/SoundSync_Cpp11.cpp
	GpApp/SourceExport.cpp
	GpApp/StringUtils.cpp
	GpApp/StructuresInit.cpp
	GpApp/StructuresInit2.cpp
	GpApp/Tools.cpp
	GpApp/Transit.cpp
	GpApp/Transitions.cpp
	GpApp/Triggers.cpp
	GpApp/Trip.cpp
	GpApp/Utilities.cpp
	GpApp/WindowUtils.cpp
	)

target_compile_options(GpApp PRIVATE -Wno-multichar)

target_include_directories(GpApp PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GpCommon>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PortabilityLayer>
	)

target_link_libraries(GpApp PortabilityLayer)

if(CMAKE_HOST_UNIX)
	add_executable(AerofoilX
		AerofoilPortable/GpSystemServices_POSIX.cpp
		AerofoilPortable/GpThreadEvent_Cpp11.cpp
        AerofoilPortable/GpLogDriver_Clog.cpp
		AerofoilSDL/GpAudioDriver_SDL2.cpp
		AerofoilSDL/GpDisplayDriver_SDL_GL2.cpp
		AerofoilSDL/GpInputDriver_SDL_Gamepad.cpp
		AerofoilSDL/ShaderCode/CopyQuadP.cpp
		AerofoilSDL/ShaderCode/DrawQuad32P.cpp
		AerofoilSDL/ShaderCode/DrawQuadPaletteP.cpp
		AerofoilSDL/ShaderCode/DrawQuadV.cpp
		AerofoilSDL/ShaderCode/ScaleQuadP.cpp
		AerofoilX/GpMain_SDL_X.cpp
		AerofoilX/GpLogDriver_X.cpp
		AerofoilX/GpSystemServices_X.cpp
		AerofoilX/GpFileSystem_X.cpp
		)
		
	target_include_directories(AerofoilX PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Common>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GpCommon>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GpShell>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/AerofoilSDL>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/AerofoilPortable>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PortabilityLayer>
		${SDL2_INCLUDE_DIRS}
		)

	target_link_libraries(AerofoilX ${SDL2_LIBRARIES} GpApp GpShell)
endif()


install (TARGETS AerofoilX)
