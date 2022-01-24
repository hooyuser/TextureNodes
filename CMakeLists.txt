cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 23)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_NAME ${PROJECT_NAME})
project(${PROJECT_NAME})

add_subdirectory(extern)

file(GLOB_RECURSE  SRC
    "source/*.cpp"
)

file(GLOB_RECURSE HEADER
    "source/*.h"
)

file(GLOB_RECURSE SHADERS
    "assets/glsl_shaders/*.vert"
    "assets/glsl_shaders/*.frag"
)

add_executable(${PROJECT_NAME}
    ${SRC}
    ${HEADER}
    ${SHADERS}
)

add_custom_command(TARGET ${PROJECT_NAME}
                   PRE_BUILD
                   COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMAND compile_spirv.bat
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
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${EXTERN_DIR}/vulkan/Lib/vulkan-1.lib
        ${EXTERN_DIR}/glfw/lib-vc2019/glfw3.lib
        imgui
        imgui-node-editor
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

