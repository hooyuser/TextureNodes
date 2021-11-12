set(PROJECT_NAME
    imgui
)

project(${PROJECT_NAME})

set(IMGUI_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
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
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/Lib/vulkan-1.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2019/glfw3.lib
)