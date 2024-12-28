cmake_minimum_required(VERSION 3.14)
project(g13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-std=c++20")

# Setup output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Boost
find_package(Boost 1.83.0 COMPONENTS log system program_options REQUIRED)

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
include_directories(./src/main)
file(GLOB_RECURSE SRC_FILES "./src/main/*.h" "./src/main/*.hpp" "./src/main/*.cc" "./src/main/*.cpp")
add_executable(g13d ${SRC_FILES})
target_link_libraries (g13d usb-1.0 ${Boost_LIBRARIES} evdev pthread pugixml)

# Install
install(TARGETS g13d pbm2lpbm DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)