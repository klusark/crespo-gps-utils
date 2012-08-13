LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := utils/bcm4751_test.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := bcm4751_test
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := utils/bcm4751_hal.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += hardware/libhardware/include/

LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware libhardware_legacy 
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := bcm4751_hal
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := utils/bcm4751_daemon.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := liblog libcutils 
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := bcm4751_daemon
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := utils/bcm4751_lib.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := liblog libcutils 
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := bcm4751_lib
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := bcm4751_gpsd.c meif.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := bcm4751_gpsd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
