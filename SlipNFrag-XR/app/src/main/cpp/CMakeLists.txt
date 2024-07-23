cmake_minimum_required(VERSION 3.22)

project(SlipNFrag-XR)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../../../../..")
set(LOCAL_DIR ".")
set(ID1_DIR "${ROOT_DIR}/id1")
set(RENDERER_DIR "${ROOT_DIR}/renderer")
set(SND_FLOAT_DIR "${ROOT_DIR}/snd_float")
set(STB_DIR "${ROOT_DIR}/stb")
set(OPENXR_DIR "${ROOT_DIR}/OpenXR-SDK-release-1.1.38")
set(ADRENO_DIR "${ROOT_DIR}/VkAdrenoLayer_v1.4")
set(ANDROID_BINARIES_DIR "${ROOT_DIR}/android-binaries-1.3.283.0")
set(ASAN_DIR "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/18/lib/linux")

option(BUILD_WITH_ASAN "Build with Address Sanitizer" OFF)
option(BUILD_WITH_VALIDATION "Build with Vulkan validation layer" OFF)
option(BUILD_WITH_ADRENO "Build with Adreno validation / performance layer" OFF)

file(GLOB LOCAL_SOURCE "${LOCAL_DIR}/*.cpp")
file(GLOB ID1_SOURCE "${ID1_DIR}/*.cpp")
file(GLOB RENDERER_SOURCE "${RENDERER_DIR}/*.cpp")
file(GLOB SND_FLOAT_SOURCE "${SND_FLOAT_DIR}/*.cpp")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_win")
list(FILTER ID1_SOURCE EXCLUDE REGEX "snd_")

add_library(slipnfrag_native MODULE
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        ${RENDERER_SOURCE}
        ${SND_FLOAT_SOURCE}
)

target_compile_options(slipnfrag_native PRIVATE
        -Wno-unused-parameter
        -Wno-trigraphs
        -DANDROID_STL=c++_shared
        -DXR_USE_PLATFORM_ANDROID=1
        -DXR_USE_GRAPHICS_API_VULKAN=1
        "-DUSE_OPENXR_VERSION=XR_MAKE_VERSION(1, 0, 34)"
)

target_link_options(slipnfrag_native PRIVATE
        -u ANativeActivity_onCreate
)

if(BUILD_WITH_ASAN)
    target_compile_options(slipnfrag_native PRIVATE -fsanitize=address -fno-omit-frame-pointer)

    target_link_options(slipnfrag_native PRIVATE -fsanitize=address)
endif()

add_library(slipnfrag_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

find_library(ANDROID_LIBRARY NAMES android)
find_library(ANDROID_LOG_LIBRARY NAMES log)
find_library(VULKAN_LIBRARY NAMES vulkan)

find_package(OpenXR REQUIRED CONFIG)

target_include_directories(slipnfrag_native PRIVATE
        ${OPENXR_DIR}/include
        ${OPENXR_DIR}/src
        ${VULKAN_INCLUDE_DIRS}
        ${LOCAL_DIR}
        ${ID1_DIR}
        ${RENDERER_DIR}
        ${STB_DIR}
        ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(slipnfrag_native
        slipnfrag_app_glue
        ${ANDROID_LIBRARY}
        ${ANDROID_LOG_LIBRARY}
        ${VULKAN_LIBRARY}
        OpenXR::openxr_loader
        OpenSLES
)

if(BUILD_WITH_VALIDATION)
    add_library(vulkan_validation SHARED IMPORTED)

    set_target_properties(vulkan_validation PROPERTIES IMPORTED_LOCATION
            ${ANDROID_BINARIES_DIR}/${ANDROID_ABI}/libVkLayer_khronos_validation.so
    )

    target_link_libraries(slipnfrag_native
            vulkan_validation
    )
endif()

if(BUILD_WITH_ASAN)
    add_library(asan SHARED IMPORTED)

    set_target_properties(asan PROPERTIES IMPORTED_LOCATION
            ${ASAN_DIR}/libclang_rt.asan-aarch64-android.so
    )

    target_link_libraries(slipnfrag_native
            asan
    )
endif()

if(BUILD_WITH_ADRENO)
    add_library(adreno SHARED IMPORTED)

    set_target_properties(adreno PROPERTIES IMPORTED_LOCATION
            ${ADRENO_DIR}/libs/${ANDROID_ABI}/libVkLayer_adreno.so
    )

    target_link_libraries(slipnfrag_native
            adreno
    )
endif()
