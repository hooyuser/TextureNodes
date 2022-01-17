set(PROJECT_NAME
    imgui-node-editor
)

project(${PROJECT_NAME})

set(PROJECT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(EXTERN_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

file(GLOB_RECURSE SRC
    "${PROJECT_DIR}/*.cpp"
)

add_library(${PROJECT_NAME} STATIC
    ${SRC}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_DIR}
        ${EXTERN_DIR}/imgui
)