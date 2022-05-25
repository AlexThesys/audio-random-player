cmake_minimum_required (VERSION 3.10)
project (audio_test)

include_directories("portaudio/include")
include_directories("AudioFile")

file(GLOB SOURCES "src/*.*")

set ( PROJECT_LINK_LIBS portaudio_static_x64.lib )
link_directories("portaudio/lib")

add_executable (audio_test ${SOURCES})

target_link_libraries(audio_test ${PROJECT_LINK_LIBS} )