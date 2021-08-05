file(GLOB LOCAL_SOURCE "*.cpp")
file(GLOB ID1_SOURCE "../../../../../../../id1/*.cpp")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_win")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(slipnfrag_native MODULE
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        $<TARGET_OBJECTS:android_native_app_glue>
)

add_library(ovr_openxr_loader SHARED IMPORTED)
set_property(TARGET ovr_openxr_loader PROPERTY IMPORTED_LOCATION 
        ${PROJECT_SOURCE_DIR}/../ovr_openxr_mobile_sdk_1.0.14/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
)

target_compile_options(slipnfrag_native PRIVATE -Wno-unused-parameter)

target_compile_options(slipnfrag_native PUBLIC -fsanitize=address -fno-omit-frame-pointer)

set_target_properties(slipnfrag_native PROPERTIES LINK_FLAGS -fsanitize=address)

add_dependencies(slipnfrag_native generate_openxr_header)

target_include_directories(slipnfrag_native PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/include
        ${Vulkan_INCLUDE_DIRS}
        ../../../../../../../id1
        ../../../../../../../stb
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

add_library(asan SHARED IMPORTED)
set_property(TARGET asan PROPERTY IMPORTED_LOCATION
        ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/lib64/clang/9.0.9/lib/linux/libclang_rt.asan-aarch64-android.so
)

target_link_libraries(slipnfrag_native debug 
        vulkan_validation
        asan
)
