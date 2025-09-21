cmake_minimum_required(VERSION 3.22)

project(SlipNFrag-PCXR)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/..")
set(LOCAL_DIR ".")
set(ID1_DIR "${ROOT_DIR}/id1")
set(RENDERER_DIR "${ROOT_DIR}/renderer")
set(SND_FLOAT_DIR "${ROOT_DIR}/snd_float")
set(SHADERS_DIR "${ROOT_DIR}/shaders")
set(STB_DIR "${ROOT_DIR}/stb")
set(OPENXR_DIR "${ROOT_DIR}/OpenXR-SDK-release-1.1.52")

file(GLOB LOCAL_SOURCE "${LOCAL_DIR}/*.cpp")
file(GLOB ID1_SOURCE "${ID1_DIR}/*.cpp")
file(GLOB RENDERER_SOURCE "${RENDERER_DIR}/*.cpp")
file(GLOB SND_FLOAT_SOURCE "${SND_FLOAT_DIR}/*.cpp")
file(GLOB SHADERS_SOURCE "${SHADERS_DIR}/*.vert" "${SHADERS_DIR}/*.frag")
file(GLOB SLIPNFRAG_XR_ASSETS "${ROOT_DIR}/SlipNFrag-XR/app/src/main/assets/*")

list(FILTER ID1_SOURCE EXCLUDE REGEX "net_udp")
list(FILTER ID1_SOURCE EXCLUDE REGEX "snd_")

add_executable(slipnfrag_pcxr
        ${LOCAL_SOURCE}
        ${ID1_SOURCE}
        ${RENDERER_SOURCE}
        ${SND_FLOAT_SOURCE}
        ${SHADERS_SOURCE}
)

target_compile_options(slipnfrag_pcxr PRIVATE
        /Zc:wchar_t 
        /Zc:forScope
        -DXR_USE_PLATFORM_WIN32=1
        -DXR_USE_GRAPHICS_API_VULKAN=1
        "-DUSE_OPENXR_VERSION=XR_MAKE_VERSION(1, 0, 34)"
        "-DUSE_VULKAN_VERSION=VK_MAKE_API_VERSION(0, 1, 1, 0)"
)

add_library(openxr_loader STATIC IMPORTED)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set_property(TARGET openxr_loader PROPERTY IMPORTED_LOCATION 
            ${OPENXR_DIR}/out/install/x64-Debug/lib/openxr_loaderd.lib
    )
else()
    set_property(TARGET openxr_loader PROPERTY IMPORTED_LOCATION 
            ${OPENXR_DIR}/out/install/x64-Release/lib/openxr_loader.lib
    )
endif()

find_package(Vulkan REQUIRED)

function(compile_glsl_use_extension run_target_name)

    set(glsl_output_files "")
    foreach(in_file IN LISTS ARGN)
        get_filename_component(glsl_name ${in_file} NAME_WE)
        get_filename_component(glsl_stage ${in_file} EXT)
        string(REPLACE "." "" glsl_stage ${glsl_stage})
        set(out_file ${CMAKE_CURRENT_BINARY_DIR}/slipnfrag_assets/shaders/${glsl_name}.${glsl_stage}.spv)
        add_custom_command(
            OUTPUT ${out_file}
            COMMAND Vulkan::glslc -fshader-stage=${glsl_stage} ${in_file} -o ${out_file}
            DEPENDS ${in_file}
        )
        list(APPEND glsl_output_files ${out_file})
    endforeach()
    add_custom_target(${run_target_name} ALL DEPENDS ${glsl_output_files})

endfunction()

compile_glsl_use_extension(run_slipnfrag_pcxr_glsl_compiles ${SHADERS_SOURCE})

add_dependencies(slipnfrag_pcxr
    run_slipnfrag_pcxr_glsl_compiles
)

target_include_directories(slipnfrag_pcxr PRIVATE
        ${OPENXR_DIR}/include
        ${OPENXR_DIR}/src
        Vulkan::Headers
        ${LOCAL_DIR}
        ${ID1_DIR}
        ${RENDERER_DIR}
        ${STB_DIR}
)

target_link_libraries(slipnfrag_pcxr PRIVATE 
        openxr_loader
        winmm.lib
        Vulkan::Vulkan
)

if(TARGET openxr-gfxwrapper)
    target_link_libraries(slipnfrag_pcxr openxr-gfxwrapper)
endif()

target_compile_features(slipnfrag_pcxr PUBLIC cxx_std_17)

target_compile_definitions(slipnfrag_pcxr PRIVATE _CRT_SECURE_NO_WARNINGS)

add_custom_command(
    TARGET slipnfrag_pcxr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${SLIPNFRAG_XR_ASSETS}
            ${CMAKE_CURRENT_BINARY_DIR}/slipnfrag_assets)
