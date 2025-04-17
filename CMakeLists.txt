cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/raylib")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/raylib_download")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/ode_download")

# set lib and include paths
set(RL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/raylib)  # raylib is inside the "include" dir
set(ODE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ode/include)    # include path for ode is in a subdir
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add the current source directory to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

include(ExternalProject)

# For statically linking raylib
# set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a)
# For dynamically linking raylib
set(RAYLIB_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.so)
# Define ODE_LIB as the destination path
set(ODE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libode.so)

# set ON to download and compile Raylib lib
set(FETCH_RL ON)
set(RAYLIB_URL_PREFIX https://github.com/raysan5/raylib/releases/download/5.0)
set(RAYLIB_DOWNLOAD_FILENAME raylib-5.0_linux_amd64.tar.gz)
set(RAYLIB_FOLDER_NAME raylib-5.0_linux_amd64)
set(RAYLIB_VERSION 5.0.0)
if(FETCH_RL)
    # Download Raylib
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/raylib_download/${RAYLIB_DOWNLOAD_FILENAME}")
        message(STATUS "Downloading Raylib...")
        file(DOWNLOAD
            ${RAYLIB_URL_PREFIX}/${RAYLIB_DOWNLOAD_FILENAME}
            ${CMAKE_SOURCE_DIR}/lib/${RAYLIB_DOWNLOAD_FILENAME}
            SHOW_PROGRESS
        )
    endif()
    # Extract the archive
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/lib/${RAYLIB_DOWNLOAD_FILENAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/raylib_download
    )

    # Copy extracted files to the appropriate locations
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/raylib_download/${RAYLIB_FOLDER_NAME}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/raylib"
    )

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/raylib_download/${RAYLIB_FOLDER_NAME}/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    )
endif()

# Add an option to enable the ODE library
option(ENABLE_ODE "Enable building ODE library" ON)
set(ODE_URL_PREFIX https://bitbucket.org/odedevs/ode/downloads)
set(ODE_DOWNLOAD_FILENAME ode-0.16.5.tar.gz)
set(ODE_FOLDER_NAME ode-0.16.5)
set(ODE_VERSION 0.16.5)

if(ENABLE_ODE)
    # Download and extract ODE only if it doesn't exist
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ode_download/${ODE_FOLDER_NAME}")
        message(STATUS "Downloading ODE...")

        # Download the ODE archive
        file(DOWNLOAD
            ${ODE_URL_PREFIX}/${ODE_DOWNLOAD_FILENAME}
            ${CMAKE_SOURCE_DIR}/ode_download/${ODE_DOWNLOAD_FILENAME}
            SHOW_PROGRESS
        )

        # Extract the archive
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/ode_download/${ODE_DOWNLOAD_FILENAME}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ode_download
        )
    endif()

    # External project for building ODE
    ExternalProject_Add(
        ODE
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ode_build
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/ode_download/${ODE_FOLDER_NAME}
        CMAKE_ARGS -DODE_WITH_OU=ON
                  -DODE_DOUBLE_PRECISION=ON
                  -DODE_WITH_LIBCCD=ON
                  -DODE_WITH_LIBCCD_BOX_CYL=ON
                  -DODE_WITH_DEMOS=OFF
                  -DODE_WITH_TESTS=OFF
                  -DBUILD_SHARED_LIBS=ON
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
        INSTALL_COMMAND ""  # Skip the install step
    )

    # Define the build location (source for copy)
    set(ODE_BUILD_LIB "${CMAKE_CURRENT_BINARY_DIR}/libode.so.${ODE_VERSION}")
    add_custom_command(
        TARGET ODE
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${ODE_BUILD_LIB}"  # Source path
            "${ODE_LIB}"        # Destination path
        COMMENT "Copying ODE library to project lib directory"
    )
endif()

# Source files
set(PROJECT_SOURCES
    src/main.cpp
    src/raylibODE.cpp
    src/gameTimer.cpp
)

# Add executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Optional flags
set(CMAKE_BUILD_TYPE "Release")
# for LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add dependency on ODE to ensure it's built before the main project
if(ENABLE_ODE)
    add_dependencies(${PROJECT_NAME} ODE)
endif()

# Set libraries to link
set(PROJECT_LIBRARIES
    ${RAYLIB_LIB}
    ${ODE_LIB}
    m
    pthread
)

# Include and link
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

# For Visual Studio solution
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Set the target as the default in Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
