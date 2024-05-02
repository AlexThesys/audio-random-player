cmake_minimum_required (VERSION 3.10)
project (audio_random_player)

# set options before add_subdirectory
# available options : TRACY_ENABLE , TRACY_ON_DEMAND , TRACY_NO_BROADCAST , TRACY_NO_CODE_TRANSFER , ...
option( TRACY_ENABLE "" ON)
option( TRACY_ON_DEMAND "" ON)
add_subdirectory (tracy) # target: TracyClient or alias Tracy :: TracyClient

#option( PROFILER_ENABLED "" ON)

include_directories("portaudio/include")
include_directories("AudioFile")
include_directories("glm")
include_directories("glfw/include")
include_directories("glew/include")
include_directories("tracy/public/tracy")

file(GLOB SOURCES "src/*.h" "src/*.cpp" tracy/public/tracy.cpp)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(DEST_DIR ${CMAKE_BINARY_DIR})
file(GLOB RESOURCE_DIR "shaders/*.glsl")
file(COPY ${RESOURCE_DIR} DESTINATION ${DEST_DIR}/shaders)

set( PROJECT_LINK_LIBS portaudio.lib glfw3.lib glew32s.lib opengl32.lib Tracy::TracyClient )
link_directories("portaudio/lib")
link_directories("glfw/lib")
link_directories("glew/lib/Release/x64")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS})

target_include_directories(${PROJECT_NAME} PRIVATE ${DEST_DIR})