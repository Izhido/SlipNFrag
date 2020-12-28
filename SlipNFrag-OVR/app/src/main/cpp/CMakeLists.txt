cmake_minimum_required(VERSION 3.4.1)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(vrapi SHARED IMPORTED)

set_target_properties(vrapi PROPERTIES IMPORTED_LOCATION ../../../../../../ovr_sdk_mobile_1.40.0/VrApi/Libs/Android/${ANDROID_ABI}/Release/libvrapi.so)

add_library(stb STATIC IMPORTED)

set_target_properties(stb PROPERTIES IMPORTED_LOCATION ../../../../../../ovr_sdk_mobile_1.40.0/3rdParty/stb/lib/android/${ANDROID_ABI}/libstb.a)

file(GLOB SRC_FILES *.cpp)
file(GLOB ID1_FILES ../../../../../id1/*.cpp)

list(FILTER ID1_FILES EXCLUDE REGEX "net_win")

add_library(native-activity SHARED ${SRC_FILES} ${ID1_FILES} ../../../../../SlipNFrag/cd_null.cpp)

target_include_directories(native-activity PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
target_include_directories(native-activity PRIVATE ../../../../../ovr_sdk_mobile_1.40.0/VrApi/Include)
target_include_directories(native-activity PRIVATE ../../../../../ovr_sdk_mobile_1.40.0/3rdParty/khronos/vulkan_1.1.100.0)
target_include_directories(native-activity PRIVATE ../../../../../id1)
target_include_directories(native-activity PRIVATE ../../../../../ovr_sdk_mobile_1.40.0/3rdParty/stb/src)

target_link_libraries(native-activity android native_app_glue log vrapi stb OpenSLES)
