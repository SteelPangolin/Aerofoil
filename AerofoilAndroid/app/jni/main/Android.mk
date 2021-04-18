LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include	\
	$(LOCAL_PATH)/../GpShell	\
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../AerofoilPortable	\
	$(LOCAL_PATH)/../AerofoilSDL	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../PortabilityLayer

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0

# Add your application source files here...
LOCAL_SRC_FILES := \
	GpMain_SDL_Android.cpp	\
	GpSystemServices_Android.cpp	\
	GpFileSystem_Android.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_STATIC_LIBRARIES := GpShell AerofoilPortable AerofoilSDL GpApp

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
