cmake_minimum_required(VERSION 3.27.5)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Check platforms
if (CMAKE_HOST_WIN32)
    set(WINDOWS 1)
elseif(CMAKE_HOST_APPLE)
    set(MACOS 1)
elseif(CMAKE_HOST_UNIX)
    set(LINUX 1)
endif()

# Output to terminal
if(CMAKE_CL_64)
    MESSAGE( STATUS "-------- 64-bit ---------")
else()
    MESSAGE( STATUS "-------- 32-bit ---------")
endif()
# Check compilers
MESSAGE( STATUS "CXX Compiler ----> ${CMAKE_CXX_COMPILER_ID}")

# set lib and include paths
set(libs)
set(RL_INCLUDE_DIR include/raylib)#raylib is inside the "include" dir
set(ODE_INCLUDE_DIR ode/include)#include path for ode is in a subdir
#TODO move it later to same include path as raylib
# Also find a way to fetch or find_package for ODE
set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/ode_double.lib)

#Decide whether to fetch libs for Raylib
option(FETCH_RL OFF)
if(FETCH_RL)
    ##----------Fetch raylib----------#
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
    set(RAYLIB_LIB raylib)
else()
  # Manually set Raylib libs
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(RAYLIB_LIB ${CMAKE_SOURCE_DIR}/lib/libraylib.a)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(RAYLIB_LIB ${CMAKE_SOURCE_DIR}/lib/libraylib.a)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(RAYLIB_LIB ${CMAKE_SOURCE_DIR}/lib/raylib.lib)
  endif()
endif()
unset(FETCH_RL CACHE)

# Libraries
list(APPEND libs ${RAYLIB_LIB} ${ODE_LIB} opengl32)
if(WINDOWS)
# For Windows: WinMM lib is only used to get a hi-res timer and it can be avoided commenting SUPPORT_WINMM_HIGHRES_TIMER
list(APPEND libs gdi32 winmm)
endif(WINDOWS)

# Source files
set(PROJECT_SOURCES src/main.cpp src/raylibODE.cpp)

# Add executable
add_executable(${PROJECT_NAME})

# Optional flags
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include and link
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RL_INCLUDE_DIR}
    ${ODE_INCLUDE_DIR}
  )
target_link_libraries(${PROJECT_NAME} PRIVATE ${libs})

# Optional: Compiler-specific silenced warnings
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

#For Visual Studio solution
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
# Set the target as the default in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
