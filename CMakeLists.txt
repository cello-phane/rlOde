cmake_minimum_required(VERSION 3.27.5)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Output to terminal
if(CMAKE_CL_64)
    MESSAGE( STATUS "-------- 64-bit ---------")
else()
    MESSAGE( STATUS "-------- 32-bit ---------")
endif()
# Check compilers
MESSAGE( STATUS "CXX Compiler ----> ${CMAKE_CXX_COMPILER_ID}")

# set lib and include paths
set(RL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/raylib)#raylib is inside the "include" dir
set(ODE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ode/include)#include path for ode is in a subdir

# set ON to download and compile Raylib lib
set(FETCH_RL OFF)
if(NOT FETCH_RL)
find_library(raylib libraylib)
  #-----Manually set Raylib-----#
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/raylib.lib)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/raylib.lib)
  else()
    set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a)
  endif()
else()
  #---------Fetch Raylib--------#
  include(FetchContent)
  set(FETCHCONTENT_QUIET FALSE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(raylib)
  set(RAYLIB_LIB raylib)
endif()

#----------Manually set ODE---------#
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/ode_double.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/ode_double.lib)
else()
  set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libode.a)
endif()

list(APPEND PROJECT_LIBRARIES ${RAYLIB_LIB} ${ODE_LIB} stdc++ m)

# Source files
set(PROJECT_SOURCES src/main.cpp src/raylibODE.cpp)

# Add executable
add_executable(${PROJECT_NAME})

# Optional flags
set(CMAKE_BUILD_TYPE "Release")
# for LSP but this may be deprecated depending on cmake version
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

# Include and link
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RL_INCLUDE_DIR}
    ${ODE_INCLUDE_DIR}
  )

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LIBRARIES})

# Optional: Compiler-specific silence warnings
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
