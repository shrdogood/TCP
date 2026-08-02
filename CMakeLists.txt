cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "D:/mingw64/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1/mingw64/bin/g++.exe")
project(TcpDemo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server server.cpp)
if(WIN32)
    target_link_libraries(server ws2_32 pthread)
endif()

add_executable(client client.cpp)
if(WIN32)
    target_link_libraries(client ws2_32)
endif()