cmake_minimum_required(VERSION 3.10)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../../../../..")
set(ID1_DIR "${ROOT_DIR}/id1")
set(STB_DIR "${ROOT_DIR}/stb")
set(OPENXR_DIR "${ROOT_DIR}/OpenXR-SDK-release-1.0.22")
set(OVR_OPENXR_DIR "${ROOT_DIR}/ovr_openxr_mobile_sdk_38.0")
set(ADRENO_DIR "${ROOT_DIR}/VkAdrenoLayer_v1.2")
set(ANDROID_BINARIES_DIR "${ROOT_DIR}/android-binaries-1.3.204.1")
set(ASAN_DIR "${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/lib64/clang/14.0.1/lib/linux")

option(BUILD_WITH_ASAN "Build with Address Sanitizer" OFF)
option(BUILD_WITH_VALIDATION "Build with Vulkan validation layer" OFF)
option(BUILD_WITH_ADRENO "Build with Adreno validation / performance layer" OFF)

file(GLOB LOCAL_SOURCE "*.cpp")
file(GLOB ID1_SOURCE "${ID1_DIR}/*.cpp")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_win")
list(FILTER ID1_SOURCE EXCLUDE REGEX "snd_")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(
        -DXR_USE_PLATFORM_ANDROID=1
        -DXR_USE_GRAPHICS_API_VULKAN=1
)

add_library(slipnfrag_native MODULE
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

add_library(ovr_openxr_loader SHARED IMPORTED)
set_property(TARGET ovr_openxr_loader PROPERTY IMPORTED_LOCATION 
        ${OVR_OPENXR_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)

target_compile_options(slipnfrag_native PRIVATE -Wno-unused-parameter)

if(BUILD_WITH_ASAN)
    target_compile_options(slipnfrag_native PUBLIC -fsanitize=address -fno-omit-frame-pointer)

    set_target_properties(slipnfrag_native PROPERTIES LINK_FLAGS -fsanitize=address)
endif()

find_library(ANDROID_LIBRARY NAMES android)
find_library(ANDROID_LOG_LIBRARY NAMES log)
find_library(VULKAN_LIBRARY NAMES vulkan)

target_include_directories(slipnfrag_native PRIVATE
        ${OPENXR_DIR}/include
        ${OPENXR_DIR}/src
        ${VULKAN_INCLUDE_DIRS}
        ${ID1_DIR}
        ${STB_DIR}
        ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(slipnfrag_native
        ${ANDROID_LIBRARY}
        ${ANDROID_LOG_LIBRARY}
        ${VULKAN_LIBRARY}
        ovr_openxr_loader
        OpenSLES
)

if(BUILD_WITH_VALIDATION)
    add_library(vulkan_validation SHARED IMPORTED)

    set_target_properties(vulkan_validation PROPERTIES IMPORTED_LOCATION
            ${ANDROID_BINARIES_DIR}/${ANDROID_ABI}/libVkLayer_khronos_validation.so
    )

    target_link_libraries(slipnfrag_native vulkan_validation)
endif()

if(BUILD_WITH_ASAN)
    add_library(asan SHARED IMPORTED)

    set_target_properties(asan PROPERTIES IMPORTED_LOCATION
            ${ASAN_DIR}/libclang_rt.asan-aarch64-android.so
    )

    target_link_libraries(slipnfrag_native asan)
endif()

if(BUILD_WITH_ADRENO)
    add_library(adreno SHARED IMPORTED)

    set_target_properties(adreno PROPERTIES IMPORTED_LOCATION
            ${ADRENO_DIR}/libs/${ANDROID_ABI}/libVkLayer_adreno.so
    )

    target_link_libraries(slipnfrag_native adreno)
endif()
