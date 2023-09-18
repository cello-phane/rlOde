cmake_minimum_required(VERSION 3.27.5)

set(PROJECT_NAME "rlOde")

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Add source files
set(SRC_FILES src/main.cpp src/raylibODE.cpp)
set(ODE_DIR ode)
add_executable(${PROJECT_NAME} ${SRC_FILES})

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    include/raylib
    ${ODE_DIR}/include
  )

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE
    /wd4244 /wd4245 /wd4305 /wd4310
    /DPLATFORM_DESKTOP
  )
  set(LIB_RAYLIB_FILE raylib.lib)
  set(LIB_ODE_FILE    ode_double.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-conversion -Wno-sign-conversion -Wno-shorten-64-to-32 -Wno-cast-qual
    -DPLATFORM_DESKTOP
  )
  set(LIB_RAYLIB_FILE libraylib.a)
  set(LIB_ODE_FILE    ode_double.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-conversion -Wno-sign-conversion -Wno-cast-qual
    -DPLATFORM_DESKTOP
  )
  set(LIB_RAYLIB_FILE libraylib.a)
  set(LIB_ODE_FILE    ode_double.lib)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIB_RAYLIB_FILE})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIB_ODE_FILE})
# WinMM is only used to get a hi-res timer and it can be avoided commenting SUPPORT_WINMM_HIGHRES_TIMER config flag.
target_link_libraries(${PROJECT_NAME} PRIVATE winmm.lib)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
# Set the target as the default in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
