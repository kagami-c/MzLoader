cmake_minimum_required(VERSION 3.0)
project(MzLoader CXX)

# global include directories
include_directories(include/
                    src/
                    3rdparty/rapidxml-1.13
                    3rdparty/zlib-1.2.8
                    3rdparty/libb64-1.2.1/include
                    3rdparty/googletest-release-1.7.0/include)

# build zlib
set(CMAKE_DEBUG_POSTFIX "d")
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

set(ZLIB_PUBLIC_HDRS
    3rdparty/zlib-1.2.8/zconf.h
    3rdparty/zlib-1.2.8/zlib.h
)
set(ZLIB_PRIVATE_HDRS
    3rdparty/zlib-1.2.8/crc32.h
    3rdparty/zlib-1.2.8/deflate.h
    3rdparty/zlib-1.2.8/gzguts.h
    3rdparty/zlib-1.2.8/inffast.h
    3rdparty/zlib-1.2.8/inffixed.h
    3rdparty/zlib-1.2.8/inflate.h
    3rdparty/zlib-1.2.8/inftrees.h
    3rdparty/zlib-1.2.8/trees.h
    3rdparty/zlib-1.2.8/zutil.h
)
set(ZLIB_SRCS
    3rdparty/zlib-1.2.8/adler32.c
    3rdparty/zlib-1.2.8/compress.c
    3rdparty/zlib-1.2.8/crc32.c
    3rdparty/zlib-1.2.8/deflate.c
    3rdparty/zlib-1.2.8/gzclose.c
    3rdparty/zlib-1.2.8/gzlib.c
    3rdparty/zlib-1.2.8/gzread.c
    3rdparty/zlib-1.2.8/gzwrite.c
    3rdparty/zlib-1.2.8/inflate.c
    3rdparty/zlib-1.2.8/infback.c
    3rdparty/zlib-1.2.8/inftrees.c
    3rdparty/zlib-1.2.8/inffast.c
    3rdparty/zlib-1.2.8/trees.c
    3rdparty/zlib-1.2.8/uncompr.c
    3rdparty/zlib-1.2.8/zutil.c
)

# add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
# set_target_properties(zlibstatic PROPERTIES LINKER_LANGUAGE C)

# build libb64
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

set(LIBB64_SRC 3rdparty/libb64-1.2.1/src/cdecode 3rdparty/libb64-1.2.1/src/cencode)
# add_library(libb64 STATIC ${LIBB64_SRC})

# build MzLoader
add_library(mzloader STATIC src/MzLoader.cpp ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS} ${LIBB64_SRC})
# target_link_libraries(mzloader PUBLIC libb64 zlibstatic)

# build unittest
add_subdirectory(3rdparty/googletest-release-1.7.0)
add_executable(unittest test/Test_MzLoader.cpp)
target_link_libraries(unittest gtest gtest_main mzloader)

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRTD")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:MSVCRT")
