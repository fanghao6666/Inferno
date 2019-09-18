cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("Inferno")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

IF(APPLE)
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
ENDIF()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(external/AssetCore)
add_subdirectory(external/glfw)

set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glfw/include")
set(IMGUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/imgui")
set(IMGUIZMO_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/ImGuizmo")


include_directories("${ASSET_CORE_INCLUDES}"
					"${GLFW_INCLUDE_DIRS}"
					"${IMGUI_INCLUDE_DIRS}"
					"${IMGUIZMO_INCLUDE_DIRS}")

add_subdirectory(src)