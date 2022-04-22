cmake_minimum_required (VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_NAME ${PROJECT_NAME})
project(${PROJECT_NAME})

add_subdirectory(extern)

file(GLOB_RECURSE SRC
    "source/*.cpp"
)

file(GLOB_RECURSE HEADER
    "source/*.h"
)

file(GLOB_RECURSE SHADERS
    "assets/glsl_shaders/*.vert"
    "assets/glsl_shaders/*.frag"
    "assets/glsl_shaders/*.comp"
)

find_package(AVX)
find_package(VendorIdentifier)

add_executable(${PROJECT_NAME}
    ${SRC}
    ${HEADER}
    ${SHADERS}
)

find_package(Git)
if (WIN32)
    if(Git_FOUND)
        add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND compile_modified_spirv.bat
        )
    else() 
        add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND compile_spirv.bat
        )
    endif()
endif()

set(no_copy $<NOT:$<CONFIG:Release>>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_SOURCE_DIR}/build/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E $<${no_copy}:echo> $<${no_copy}:"copy omitted for non-release build, command would have been "> copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/build/bin/assets
)

source_group("Source Files"            FILES ${SRC})
source_group("Header Files"            FILES ${HEADER})
source_group("GLSL Shaders"        FILES ${SHADERS})

set(EXTERN_DIR
    ${CMAKE_SOURCE_DIR}/extern
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${EXTERN_DIR}/vulkan/Include
        ${EXTERN_DIR}/glm
        ${EXTERN_DIR}/glfw/include
        ${EXTERN_DIR}/stb
        ${EXTERN_DIR}/tinyobjloader
        ${EXTERN_DIR}/tinygltf
        ${EXTERN_DIR}/tinyexr
        ${EXTERN_DIR}/nlohmann
        ${EXTERN_DIR}/RareCpp
        ${EXTERN_DIR}/imgui
        ${EXTERN_DIR}/imgui/backends
        ${EXTERN_DIR}/imgui-node-editor
        ${EXTERN_DIR}/magic_enum/include
        ${EXTERN_DIR}/IconsFont
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${EXTERN_DIR}/vulkan/Lib/vulkan-1.lib
        ${EXTERN_DIR}/glfw/lib-vc2019/glfw3.lib
        imgui
        imgui-node-editor
)

set(ALL_PROJECT_TARGETS ${PROJECT_NAME} imgui imgui-node-editor)
set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES VS_DPI_AWARE "On")
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /Oi /options:strict /MP)
    if (${CXX_AVX512_FOUND})
        set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES COMPILE_FLAGS "/arch:AVX512")
    elseif (${CXX_AVX2_FOUND})
        set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    endif()
    message(STATUS "CPU_VENDOR: ${CPU_VENDOR}")
    if ("${CPU_VENDOR}" STREQUAL "Intel")
        set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES COMPILE_FLAGS "/favor:INTEL64")
    elseif("${CPU_VENDOR}" STREQUAL "AMD")
        set_target_properties(${ALL_PROJECT_TARGETS} PROPERTIES COMPILE_FLAGS "/favor:AMD64")
    endif()
endif()
