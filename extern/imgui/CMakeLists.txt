set(PROJECT_NAME
    imgui
)

project(${PROJECT_NAME})

set(IMGUI_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(EXTERN_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

file(GLOB_RECURSE IMGUI_SRC
    "${IMGUI_DIR}/*.cpp"
)

add_library(${PROJECT_NAME} STATIC
    ${IMGUI_SRC}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${EXTERN_DIR}/vulkan/Include
        ${EXTERN_DIR}/glfw/include
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${EXTERN_DIR}/vulkan/Lib/vulkan-1.lib
        ${EXTERN_DIR}/glfw/lib-vc2019/glfw3.lib
)