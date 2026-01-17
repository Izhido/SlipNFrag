cmake_minimum_required(VERSION 4.0)

project(SlipNFrag-XR)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../../../../..")
set(LOCAL_DIR ".")
set(ID1_DIR "${ROOT_DIR}/id1")
set(RENDERER_DIR "${ROOT_DIR}/renderer")
set(SND_FLOAT_DIR "${ROOT_DIR}/snd_float")
set(STB_DIR "${ROOT_DIR}/stb")
set(OPENXR_DIR "${ROOT_DIR}/OpenXR-SDK-release-1.1.54")
set(MINIZIP_DIR "${ROOT_DIR}/minizip")
set(LHASA_DIR "${ROOT_DIR}/lhasa-0.5.0")

file(GLOB LOCAL_SOURCE "${LOCAL_DIR}/*.cpp")
file(GLOB ID1_SOURCE "${ID1_DIR}/*.cpp")
file(GLOB RENDERER_SOURCE "${RENDERER_DIR}/*.cpp")
file(GLOB SND_FLOAT_SOURCE "${SND_FLOAT_DIR}/*.cpp")
file(GLOB LHASA_SOURCE "${LHASA_DIR}/lib/*.c")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_win")
list(FILTER ID1_SOURCE EXCLUDE REGEX "snd_")

list(FILTER LHASA_SOURCE EXCLUDE REGEX "bit_stream_reader")
list(FILTER LHASA_SOURCE EXCLUDE REGEX "lh_new_decoder")
list(FILTER LHASA_SOURCE EXCLUDE REGEX "tree_decode")
list(FILTER LHASA_SOURCE EXCLUDE REGEX "pma_common")

add_library(slipnfrag_native MODULE
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        ${RENDERER_SOURCE}
        ${SND_FLOAT_SOURCE}
        "${MINIZIP_DIR}/unzip.c"
        "${MINIZIP_DIR}/ioapi.c"
        ${LHASA_SOURCE}
)

target_compile_options(slipnfrag_native PRIVATE
        -Wno-unused-parameter
        -Wno-trigraphs
        -DANDROID_STL=c++_shared
        -DXR_USE_PLATFORM_ANDROID=1
        -DXR_USE_GRAPHICS_API_VULKAN=1
)

target_link_options(slipnfrag_native PRIVATE
        -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode
)

find_library(ANDROID_LIBRARY NAMES android)
find_library(ANDROID_LOG_LIBRARY NAMES log)
find_library(ZLIB_LIBRARY NAMES z)

find_package(Vulkan REQUIRED)
find_package(OpenXR REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)

target_include_directories(slipnfrag_native PRIVATE
        ${OPENXR_DIR}/src
        Vulkan::Headers
        ${LOCAL_DIR}
        ${ID1_DIR}
        ${RENDERER_DIR}
        ${STB_DIR}
        ${MINIZIP_DIR}
        ${LHASA_DIR}/lib
        ${LHASA_DIR}/lib/public
)

target_link_libraries(slipnfrag_native
        game-activity::game-activity_static
        ${ANDROID_LIBRARY}
        ${ANDROID_LOG_LIBRARY}
        ${ZLIB_LIBRARY}
        Vulkan::Vulkan
        OpenXR::openxr_loader
        OpenSLES
)
