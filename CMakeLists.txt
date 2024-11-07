cmake_minimum_required(VERSION 3.27.5)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

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
# set(CPP_INCLUDE_DIR "/usr/include/c++/14.1.0/x86_64-t2-linux-gnu /usr/include/c++/14.1.0")

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

include(ExternalProject)
# Add an option to enable the ODE library
option(ENABLE_ODE "Enable building ODE library" ON)

# Optionally add the ode_download directory as a subproject
if(ENABLE_ODE)
    # Check if the ode_download directory exists
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ode_download")
        message(FATAL_ERROR "Directory ode_download does not exist.")
    endif()

    # Check if the directory contains necessary CMake files
    file(GLOB_RECURSE odefiles "${CMAKE_CURRENT_SOURCE_DIR}/ode_download/*")
    if(odefiles)
        message("Directory is not empty")
    else()
        message("Directory is empty. Downloading ODE...")
        file(DOWNLOAD https://bitbucket.org/odedevs/ode/downloads/ode-0.16.5.tar.gz
            ${CMAKE_CURRENT_SOURCE_DIR}/ode-0.16.5.tar.gz
        )
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/ode-0.16.5.tar.gz --one-top-level=ode_download --strip-components 1
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()

    # Ensure that ODE library source files are available
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ode_download/CMakeLists.txt")
        message(FATAL_ERROR "CMakeLists.txt not found in ode_download directory.")
    endif()

    # Define the output directory of the built library
    set(ODE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ode_build")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      file(GLOB ODE_LIB_FILE "${ODE_OUTPUT_DIR}/ode_double.so*")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      file(GLOB ODE_LIB_FILE "${ODE_OUTPUT_DIR}/ode_double.so*")
    else()
      file(GLOB ODE_LIB_FILE "${ODE_OUTPUT_DIR}/libode.so*")
    endif()
    
    ExternalProject_Add(
        ODE
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ode_build
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ode_download  # Set the source directory
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DODE_WITH_OU=ON
                                           -DODE_DOUBLE_PRECISION=ON
                                           -DODE_WITH_LIBCCD=ON
                                           -DODE_WITH_LIBCCD_BOX_CYL=ON
                                           -DODE_WITH_DEMOS=OFF
                                           -DODE_WITH_TESTS=OFF
                                           -DBUILD_SHARED_LIBS=ON                                           
                                           -S ${CMAKE_CURRENT_SOURCE_DIR}/ode_download
                                           -B ${CMAKE_CURRENT_BINARY_DIR}/ode_build
        BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/ode_build
        INSTALL_COMMAND ""  # Skip the install step
    )
    # Custom command to copy the shared library after the build is complete
    add_custom_command(
      TARGET ODE POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ODE_LIB_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/lib/ || true
      COMMENT "Copying ODE shared library to ${CMAKE_CURRENT_SOURCE_DIR}/lib (non-fatal)"
    )
endif()

# Set the library based on the compiler
set(ODE_LIB ${ODE_LIB_FILE})

list(APPEND PROJECT_LIBRARIES ${RAYLIB_LIB} ${ODE_LIB} stdc++ m)

# Source files
set(PROJECT_SOURCES src/main.cpp src/raylibODE.cpp src/gameTimer.cpp)

# Add executable
add_executable(${PROJECT_NAME})

# Optional flags
set(CMAKE_BUILD_TYPE "Release")
# for LSP but this may be deprecated depending on cmake version
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include and link
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RL_INCLUDE_DIR}
    ${ODE_INCLUDE_DIR}
    # ${CPP_INCLUDE_DIR}
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
