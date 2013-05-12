LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := libnatpeer
LOCAL_SRC_FILES        := ../libnatpeer.c
LOCAL_C_INCLUDES       := $(LOCAL_PATH)                        \
                          $(NDK_ROOT)/external/jansson/src     \
                          $(NDK_ROOT)/external/jansson/android
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE           := natpeer-android
LOCAL_SRC_FILES        := ../natpeer.c
LOCAL_C_INCLUDES       := $(LOCAL_PATH)                        \
                          $(NDK_ROOT)/external/jansson/src     \
                          $(NDK_ROOT)/external/jansson/android
LOCAL_STATIC_LIBRARIES := libnatpeer libjansson

include $(BUILD_EXECUTABLE)
include $(NDK_ROOT)/external/jansson/Android.mk
