cmake_minimum_required(VERSION 3.14)
project(g13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

# Setup output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

############################
# External Libraries (begin)
############################
include(FetchContent)
FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG "v1.14"
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
        SYSTEM
        EXCLUDE_FROM_ALL
)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "v1.92.8"
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG "3.4"
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
        SYSTEM
        EXCLUDE_FROM_ALL
)
FetchContent_Declare(
        fontawesome
        GIT_REPOSITORY https://github.com/FortAwesome/Font-Awesome.git
        GIT_TAG "6.7.2"
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
)
# more `FetchContent_Declare`s (if any). They should all be declared
# before any calls to FetchContent_Make_available (see docs for why).

FetchContent_MakeAvailable(pugixml)
# more `FetchContent_MakeAvailable`s (if any).

############################
# External Libraries   (end)
############################

# Image conversion utility
add_executable(pbm2lpbm src/pbm2lpbm/pbm2lpbm.cpp)

# Main driver
set(G13_MAIN_DIR "${PROJECT_SOURCE_DIR}/src/main")
file(GLOB_RECURSE G13_MAIN_SOURCES CONFIGURE_DEPENDS
        "${G13_MAIN_DIR}/*.h"
        "${G13_MAIN_DIR}/*.hpp"
        "${G13_MAIN_DIR}/*.cc"
        "${G13_MAIN_DIR}/*.cpp"
)
add_executable(g13d ${G13_MAIN_SOURCES})
target_include_directories(g13d PRIVATE ${G13_MAIN_DIR})
target_link_libraries(g13d PRIVATE usb-1.0 evdev pthread pugixml)

# Companion profile editor
option(BUILD_G13_EDITOR "Build the Dear ImGui companion profile editor" ON)
if (BUILD_G13_EDITOR)
        find_package(Vulkan REQUIRED)
        FetchContent_MakeAvailable(glfw)
        FetchContent_GetProperties(imgui)
        if (NOT imgui_POPULATED)
                FetchContent_Populate(imgui)
        endif ()
        FetchContent_GetProperties(fontawesome)
        if (NOT fontawesome_POPULATED)
                FetchContent_Populate(fontawesome)
        endif ()

        add_library(imgui STATIC
                ${imgui_SOURCE_DIR}/imgui.cpp
                ${imgui_SOURCE_DIR}/imgui_draw.cpp
                ${imgui_SOURCE_DIR}/imgui_tables.cpp
                ${imgui_SOURCE_DIR}/imgui_widgets.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
                ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        )
        target_include_directories(imgui PUBLIC
                ${imgui_SOURCE_DIR}
                ${imgui_SOURCE_DIR}/backends
                ${imgui_SOURCE_DIR}/misc/cpp
        )
        target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

        set(G13_EDITOR_DIR "${PROJECT_SOURCE_DIR}/src/editor")
        file(GLOB G13_EDITOR_SOURCES CONFIGURE_DEPENDS
                "${G13_EDITOR_DIR}/*.h"
                "${G13_EDITOR_DIR}/*.cpp"
        )
        add_executable(g13_profile_editor ${G13_EDITOR_SOURCES})
        target_include_directories(g13_profile_editor PRIVATE ${G13_EDITOR_DIR})
        target_compile_definitions(g13_profile_editor PRIVATE G13_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
        target_compile_definitions(g13_profile_editor PRIVATE G13_FA_SOLID_FONT_PATH="${fontawesome_SOURCE_DIR}/webfonts/fa-solid-900.ttf")
        target_link_libraries(g13_profile_editor PRIVATE imgui pugixml)

        configure_file(
                packaging/g13-profile-editor.desktop.in
                ${CMAKE_CURRENT_BINARY_DIR}/g13-profile-editor.desktop
                @ONLY
        )

        install(TARGETS g13_profile_editor
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        install(FILES src/editor/g13_editor_icon.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
                RENAME g13-profile-editor.png)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/g13-profile-editor.desktop
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif ()

# Install
install(TARGETS g13d pbm2lpbm DESTINATION /usr/local/bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES systemd/g13.service DESTINATION /usr/lib/systemd/user)
install(FILES 71-g13.rules DESTINATION /usr/lib/udev/rules.d)
