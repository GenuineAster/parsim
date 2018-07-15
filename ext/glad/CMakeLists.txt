project( glad )
cmake_minimum_required( VERSION 3.8 )

set( GLAD_SOURCES
	src/glad.c
)

add_library(glad STATIC ${GLAD_SOURCES})

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)


