project( parsim C CXX )
cmake_minimum_required( VERSION 3.8 )

set(GLFW_BUILD_EXAMPLES OFF CACHE STRING "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE STRING "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE STRING "" FORCE)
set(GLFW_INSTALL        OFF CACHE STRING "" FORCE)

add_subdirectory(ext/glad)
add_subdirectory(ext/glfw)

set( PARSIM_SOURCES
	src/main.cpp
)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

add_executable(parsim ${PARSIM_SOURCES})
target_link_libraries(parsim glad)
target_link_libraries(parsim glfw)
target_include_directories(parsim PRIVATE ext/glm)

