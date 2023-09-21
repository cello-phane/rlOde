cmake_minimum_required(VERSION 3.27.5)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(libs)
set(RL_INCLUDE_DIR include/raylib)#raylib is inside the "include" dir
set(ODE_INCLUDE_DIR ode/include)#include path for ode is in a subdir
#TODO move it later to same include path as raylib
# Also find a way to fetch or find_package for ODE
set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/ode_double.lib)


find_library(RAYLIB_LIB raylib)
if(NOT ${RAYLIB_LIB})
#----------Fetch raylib----------#
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games

FetchContent_Declare(
   raylib
   GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
   GIT_TAG "master"
   GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)
endif()

list(APPEND libs raylib ${ODE_LIB} opengl32 gdi32 winmm)

# Add source files
set(SRC_FILES src/main.cpp src/raylibODE.cpp)

# Add executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RL_INCLUDE_DIR}
    ${ODE_INCLUDE_DIR}
  )

# Silence warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE
    /wd4244 /wd4245 /wd4305 /wd4310
    /DPLATFORM_DESKTOP
  )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-deprecated -Wno-conversion -Wno-sign-conversion -Wno-shorten-64-to-32 -Wno-cast-qual
    -DPLATFORM_DESKTOP
  )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-deprecated -Wno-conversion -Wno-sign-conversion -Wno-cast-qual
    -DPLATFORM_DESKTOP
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${libs})
# WinMM is only used to get a hi-res timer and it can be avoided commenting SUPPORT_WINMM_HIGHRES_TIMER 
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
# Set the target as the default in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
