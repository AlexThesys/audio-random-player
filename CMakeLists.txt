cmake_minimum_required (VERSION 3.10)
project (audio_test)

include_directories("portaudio/include")
include_directories("AudioFile")
include_directories("glm")
include_directories("glfw/include")
include_directories("glew/include")

file(GLOB SOURCES "src/*.h" "src/*.cpp")

set(SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(DEST_DIR ${CMAKE_BINARY_DIR})
file(GLOB RESOURCE_DIR "shaders/*.glsl")
file(COPY ${RESOURCE_DIR} DESTINATION ${DEST_DIR}/shaders)

set( PROJECT_LINK_LIBS portaudio.lib glfw3.lib glew32s.lib opengl32.lib )
link_directories("portaudio/lib")
link_directories("glfw/lib")
link_directories("glew/lib/Release/x64")

add_executable(audio_test ${SOURCES})

target_link_libraries(audio_test ${PROJECT_LINK_LIBS} )

target_include_directories(audio_test PRIVATE ${DEST_DIR})