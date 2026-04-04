cmake_minimum_required(VERSION 3.24)

project(mide LANGUAGES CXX)

include(FetchContent)

if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking)

FetchContent_MakeAvailable(glfw)
FetchContent_GetProperties(imgui)

if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)

target_compile_definitions(imgui
  PRIVATE
    GLFW_INCLUDE_NONE)

target_link_libraries(imgui
  PUBLIC
    glfw)

add_executable(mide
  src/main.cpp
  src/app/application.cpp
  src/app/editor_shell.cpp
  src/app/theme.cpp
  src/compiler/document/document_snapshot.cpp
  src/compiler/document/line_index.cpp
  src/compiler/document/text_change.cpp
  src/compiler/syntax/lexer.cpp
  src/compiler/syntax/syntax_support.cpp
  src/compiler/syntax/syntax_tree.cpp
  src/panels/assembly_panel.cpp
  src/panels/console_panel.cpp
  src/panels/editor_panel.cpp
  src/panels/explorer_panel.cpp
  src/panels/memory_panel.cpp
  src/panels/outline_panel.cpp
  src/panels/panel_common.cpp
  src/panels/watch_panel.cpp)

target_include_directories(mide
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(mide
  PRIVATE
    GLFW_INCLUDE_NONE)

target_link_libraries(mide
  PRIVATE
    imgui
    opengl32)

if(MSVC)
  target_compile_options(mide PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(mide PRIVATE -Wall -Wextra -Wpedantic)
endif()
