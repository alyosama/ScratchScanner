LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#opencv
OPENCVROOT:= /home/aly/Android/OpenCV_sdk
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
OPENCV_LIB_TYPE:=SHARED
include ${OPENCVROOT}/sdk/native/jni/OpenCV.mk

LOCAL_LDLIBS += -llog
LOCAL_MODULE := hello

include $(BUILD_SHARED_LIBRARY)

#tess-two
TESSTWOROOT:= /home/aly/Android/tess-two
include ${TESSTWOROOT}/jni/Android.mk

LOCAL_SRC_FILES := main.c
