cmake_minimum_required( VERSION 3.0.2 )
project( XivDat )

include( cmake/compiler.cmake )

add_subdirectory(dep/boost)
add_subdirectory(dep/zlib)

add_subdirectory(src)
