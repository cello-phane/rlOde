cmake_minimum_required(VERSION 3.5)

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

# Add the current source directory to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

include(ExternalProject)

# set ON to download and compile Raylib lib
set(FETCH_RL ON)
if(FETCH_RL)
      file(DOWNLOAD 
        https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz
        ${CMAKE_SOURCE_DIR}/lib/raylib-5.0_linux_amd64.tar.gz
      )

      # Ensure the target directory exists
      file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

      # Extract the archive
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/raylib-5.0_linux_amd64.tar.gz
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/raylib-5.0_linux_amd64
        COMMAND ${CMAKE_COMMAND} -E copa_directory $WORKING_DIRECTORY/include/raylib ${CMAKE_CURRENT_SOURCE_DIR}/include/raylib
        COMMAND ${CMAKE_COMMAND} -E copy $WORKING_DIRECTORY/lib/libraylib.a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a
      )
endif()
# statically link raylib
set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a)

# Add an option to enable the ODE library
option(ENABLE_ODE "Enable building ODE library" ON)

if(ENABLE_ODE)
    # Download and extract ODE only if `ode_download` is empty
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ode_download/ode-0.16.5")
      message("ODE download directory is empty. Downloading and extracting ODE...")
      
      # Download the ODE archive
      file(DOWNLOAD 
        https://bitbucket.org/odedevs/ode/downloads/ode-0.16.5.tar.gz 
        ${CMAKE_SOURCE_DIR}/ode-0.16.5.tar.gz
      )

      # Extract the archive
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/ode-0.16.5.tar.gz
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ode_download/ode-0.16.5
      )
    endif()

    # Define the output directory of the built library
    set(ODE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ode_build")

    # External project for building
    ExternalProject_Add(
        ODE
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ode_build
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/ode_download/ode-0.16.5
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DODE_WITH_OU=ON
                                           -DODE_DOUBLE_PRECISION=ON
                                           -DODE_WITH_LIBCCD=ON
                                           -DODE_WITH_LIBCCD_BOX_CYL=ON
                                           -DODE_WITH_DEMOS=OFF
                                           -DODE_WITH_TESTS=OFF
                                           -DBUILD_SHARED_LIBS=ON                                           
                                           -S ${CMAKE_SOURCE_DIR}/ode_download/ode-0.16.5
                                           -B ${ODE_OUTPUT_DIR}
        BUILD_COMMAND ${CMAKE_COMMAND} --build ${ODE_OUTPUT_DIR}
        INSTALL_COMMAND ""  # Skip the install step
    )

    file(GLOB ODE_LIB_FILE "${ODE_OUTPUT_DIR}/libode.so*")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy ${ODE_LIB_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/lib
    )
endif()

# Link the ODE library
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
