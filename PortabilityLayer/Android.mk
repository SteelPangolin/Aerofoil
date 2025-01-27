LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := PortabilityLayer

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../rapidjson/include	\
	$(LOCAL_PATH)/../MacRomanConversion	\
	$(LOCAL_PATH)/../stb

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0 -DGP_ZLIB_BUILTIN=1

LOCAL_EXPORT_LDLIBS := -lz

# Add your application source files here...
LOCAL_SRC_FILES := \
	AntiAliasTable.cpp	\
	AppEventHandler.cpp	\
	BinHex4.cpp	\
	BitmapImage.cpp	\
	ByteSwap.cpp	\
	CFileStream.cpp	\
	DeflateCodec.cpp	\
	DialogManager.cpp	\
	DisplayDeviceManager.cpp	\
	EllipsePlotter.cpp	\
	FileBrowserUI.cpp	\
	FileManager.cpp	\
	FileSectionStream.cpp	\
	FontFamily.cpp	\
	FontManager.cpp	\
	FontRenderer.cpp	\
	GPArchive.cpp	\
	HostSuspendHook.cpp	\
	IconLoader.cpp	\
	InflateStream.cpp	\
	InputManager.cpp	\
	LinePlotter.cpp	\
	MacBinary2.cpp	\
	MacFileInfo.cpp	\
	MacFileMem.cpp	\
	MemoryManager.cpp	\
	MemReaderStream.cpp	\
	MenuManager.cpp	\
	MMBlock.cpp	\
	MMHandleBlock.cpp	\
	PLApplication.cpp	\
	PLButtonWidget.cpp	\
	PLControlDefinitions.cpp	\
	PLCore.cpp	\
	PLCTabReducer.cpp	\
	PLDialogs.cpp	\
	PLDrivers.cpp	\
	PLEditboxWidget.cpp	\
	PLEventQueue.cpp	\
	PLHacks.cpp	\
	PLHandle.cpp	\
	PLIconWidget.cpp	\
	PLImageWidget.cpp	\
	PLInvisibleWidget.cpp	\
	PLKeyEncoding.cpp	\
	PLLabelWidget.cpp	\
	PLMenus.cpp	\
	PLMovies.cpp	\
	PLNumberFormatting.cpp	\
	PLPopupMenuWidget.cpp	\
	PLQDOffscreen.cpp	\
	PLQDraw.cpp	\
	PLResourceManager.cpp	\
	PLResources.cpp	\
	PLScrollBarWidget.cpp	\
	PLSound.cpp	\
	PLStandardColors.cpp	\
	PLStringCompare.cpp	\
	PLSysCalls.cpp	\
	PLTimeTaggedVOSEvent.cpp	\
	PLWidgets.cpp	\
	QDGraf.cpp	\
	QDManager.cpp	\
	QDPictDecoder.cpp	\
	QDPictEmitContext.cpp	\
	QDPictHeader.cpp	\
	QDPixMap.cpp	\
	QDPort.cpp	\
	QDStandardPalette.cpp	\
	RandomNumberGenerator.cpp	\
	ResolveCachingColor.cpp	\
	ResourceCompiledRef.cpp	\
	ResourceFile.cpp	\
	ScanlineMask.cpp	\
	ScanlineMaskBuilder.cpp	\
	ScanlineMaskConverter.cpp	\
	ScanlineMaskIterator.cpp	\
	SimpleGraphic.cpp	\
	TextPlacer.cpp	\
	UTF8.cpp	\
	UTF16.cpp	\
	WindowDef.cpp	\
	WindowManager.cpp	\
	WorkerThread.cpp	\
	XModemCRC.cpp	\
	ZipFileProxy.cpp
	

LOCAL_STATIC_LIBRARIES := MacRomanConversion stb

include $(BUILD_STATIC_LIBRARY)
