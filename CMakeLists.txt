cmake_minimum_required(VERSION 3.9)
project(dsptest)

if (MSVC)
    set(CMAKE_CXX_FLAGS "-O2 /std:c++17") # 
else()
    set(CMAKE_CXX_FLAGS "-O3 -std=c++17 -fpermissive")
    link_libraries(pthread)
endif (MSVC)

include_directories("src/")
include_directories("src/libcorrect/")

file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

add_executable(dsptest ${SRC})


if (MSVC)
    target_include_directories(dsptest PUBLIC "C:/Program Files/PothosSDR/include/")
    target_link_directories(dsptest PUBLIC "C:/Program Files/PothosSDR/lib/")
endif (MSVC)

target_link_libraries(dsptest PUBLIC volk)
