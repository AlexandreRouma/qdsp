cmake_minimum_required(VERSION 3.9)
project(dsptest)

include_directories("src/")

add_executable(dsptest "src/main.cpp")

target_include_directories(dsptest PUBLIC "C:/Program Files/PothosSDR/include/")
target_link_directories(dsptest PUBLIC "C:/Program Files/PothosSDR/lib/")
target_link_libraries(dsptest PUBLIC volk)