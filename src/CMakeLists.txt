cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable")

file(GLOB_RECURSE INFERNO_HEADERS ${PROJECT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/src/*.cpp)

list(APPEND INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/external/imgui/imgui.cpp
                          ${PROJECT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
                          ${PROJECT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
                          ${PROJECT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
                          ${PROJECT_SOURCE_DIR}/external/ImGuizmo/ImGuizmo.cpp)

list(APPEND INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/src/external/nfd/nfd_common.c)

if (APPLE)
    list(APPEND INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/src/external/nfd/nfd_cocoa.m)
elseif (WIN32)
    list(APPEND INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/src/external/nfd/nfd_win.cpp)
elseif (LINUX)
    list(APPEND INFERNO_SOURCE ${PROJECT_SOURCE_DIR}/src/external/nfd/nfd_gtk.c)
endif()

add_executable(Inferno ${INFERNO_HEADERS} ${INFERNO_SOURCE})

target_link_libraries(Inferno AssetCoreRuntime)
target_link_libraries(Inferno glfw)
target_link_libraries(Inferno ${VULKAN_LIBRARY})

if (APPLE)
    add_custom_command(TARGET Inferno POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader $<TARGET_FILE_DIR:Inferno>/Inferno.app/Contents/Resources/assets/shader)
else()
    add_custom_command(TARGET Inferno POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader $<TARGET_FILE_DIR:Inferno>/assets/shader)
endif()

if(CLANG_FORMAT_EXE)
    add_custom_target(Inferno-clang-format-project-files COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${INFERNO_HEADERS} ${INFERNO_SOURCE})
endif()