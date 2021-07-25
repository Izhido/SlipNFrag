cmake_minimum_required(VERSION 3.4.1)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

set (OVR_SDK_MOBILE "ovr_sdk_mobile_1.46.0")

add_library(vrapi SHARED IMPORTED)
set_target_properties(vrapi PROPERTIES IMPORTED_LOCATION ../../../../../../${OVR_SDK_MOBILE}/VrApi/Libs/Android/${ANDROID_ABI}/Release/libvrapi.so)

add_library(validation SHARED IMPORTED)
set_target_properties(validation PROPERTIES IMPORTED_LOCATION ${ANDROID_NDK}/sources/third_party/vulkan/src/build-android/jniLibs/${ANDROID_ABI}/libVkLayer_khronos_validation.so)

file(GLOB SRC_FILES *.cpp)
file(GLOB ID1_FILES ../../../../../id1/*.cpp)
file(GLOB STB_FILES ../../../../../${OVR_SDK_MOBILE}/3rdParty/stb/src/*.c)

list(FILTER ID1_FILES EXCLUDE REGEX "net_win")

add_library(slipnfrag-native SHARED ${SRC_FILES} ${ID1_FILES} ${STB_FILES})

target_include_directories(slipnfrag-native PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
target_include_directories(slipnfrag-native PRIVATE ${ANDROID_NDK}/sources/third_party/vulkan/src/include)
target_include_directories(slipnfrag-native PRIVATE ../../../../../${OVR_SDK_MOBILE}/VrApi/Include)
target_include_directories(slipnfrag-native PRIVATE ../../../../../id1)
target_include_directories(slipnfrag-native PRIVATE ../../../../../${OVR_SDK_MOBILE}/3rdParty/stb/src)

target_link_libraries(slipnfrag-native android native_app_glue log vrapi OpenSLES)
target_link_libraries(slipnfrag-native debug validation)
