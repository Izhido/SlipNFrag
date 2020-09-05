
LOCAL_PATH:= $(call my-dir)

#--------------------------------------------------------
# libslipnfrag.so
#--------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE			:= slipnfrag
ID1_FILES				:= $(wildcard $(LOCAL_PATH)/../../../../../id1/*.cpp)
ID1_FILES				:= $(filter-out $(LOCAL_PATH)/../../../../../id1/net_win.cpp, $(ID1_FILES))
ID1_FILES				:= $(filter-out $(LOCAL_PATH)/../../../../../id1/net_wins.cpp, $(ID1_FILES))
LOCAL_SRC_FILES			:=  $(LOCAL_PATH)/../../../../../SlipNFrag/cd_null.cpp \
							$(LOCAL_PATH)/../../../Src/in_ovr.cpp \
							$(LOCAL_PATH)/../../../Src/main.cpp \
							$(LOCAL_PATH)/../../../Src/net_ovr.cpp \
							$(LOCAL_PATH)/../../../Src/snd_ovr.cpp \
							$(LOCAL_PATH)/../../../Src/sys_ovr.cpp \
							$(LOCAL_PATH)/../../../Src/vid_ovr.cpp
LOCAL_SRC_FILES         += $(ID1_FILES)
LOCAL_C_INCLUDES        :=	$(LOCAL_PATH)/../../../../../id1 \
							$(LOCAL_PATH)/../../../../3rdParty/stb/src
LOCAL_LDLIBS			:= -llog -landroid -lOpenSLES

LOCAL_LDFLAGS			:= -u ANativeActivity_onCreate

LOCAL_STATIC_LIBRARIES	:= stb android_native_app_glue
LOCAL_SHARED_LIBRARIES	:= vrapi

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,3rdParty/stb/build/android/jni)