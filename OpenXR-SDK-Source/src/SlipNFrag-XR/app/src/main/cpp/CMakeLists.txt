file(GLOB LOCAL_SOURCE "*.cpp")
file(GLOB ID1_SOURCE "${PROJECT_SOURCE_DIR}/../id1/*.cpp")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_win")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(slipnfrag_native MODULE
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        $<TARGET_OBJECTS:android_native_app_glue>
)

add_library(ovr_openxr_loader SHARED IMPORTED)
set_property(TARGET ovr_openxr_loader PROPERTY IMPORTED_LOCATION 
        ${PROJECT_SOURCE_DIR}/../ovr_openxr_mobile_sdk_1.0.17/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)

target_compile_options(slipnfrag_native PRIVATE -Wno-unused-parameter)

option(BUILD_WITH_ASAN "Build with Address Sanitizer" OFF)

if(BUILD_WITH_ASAN)
    target_compile_options(slipnfrag_native PUBLIC -fsanitize=address -fno-omit-frame-pointer)

    set_target_properties(slipnfrag_native PROPERTIES LINK_FLAGS -fsanitize=address)
endif()

add_dependencies(slipnfrag_native generate_openxr_header)

target_include_directories(slipnfrag_native PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/include
        ${Vulkan_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/../id1
        ${PROJECT_SOURCE_DIR}/../stb
)

target_link_libraries(slipnfrag_native
        ${ANDROID_LIBRARY}
        ${ANDROID_LOG_LIBRARY}
        ovr_openxr_loader
        openxr-gfxwrapper
        ${Vulkan_LIBRARY}
        OpenSLES
)

add_library(vulkan_validation SHARED IMPORTED)

set_target_properties(vulkan_validation PROPERTIES IMPORTED_LOCATION 
        ${ANDROID_NDK}/sources/third_party/vulkan/src/build-android/jniLibs/${ANDROID_ABI}/libVkLayer_khronos_validation.so
)

target_link_libraries(slipnfrag_native debug vulkan_validation)

if(BUILD_WITH_ASAN)
    add_library(asan SHARED IMPORTED)

    set_target_properties(asan PROPERTIES IMPORTED_LOCATION
            ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/lib64/clang/9.0.9/lib/linux/libclang_rt.asan-aarch64-android.so
    )

    target_link_libraries(slipnfrag_native asan)
endif()

option(BUILD_WITH_ADRENO "Build with Adreno validation / performance layer" OFF)

if(BUILD_WITH_ADRENO)
    add_library(adreno SHARED IMPORTED)

    set_target_properties(adreno PROPERTIES IMPORTED_LOCATION
            ${PROJECT_SOURCE_DIR}/../VkAdrenoLayer_v1.1/libs/${ANDROID_ABI}/libVkLayer_adreno.so
    )

    target_link_libraries(slipnfrag_native adreno)
endif()

