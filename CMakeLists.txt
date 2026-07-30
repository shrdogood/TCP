cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "D:/mingw64/x86_64-8.1.0-release-win32-seh-rt_v6-rev0/mingw64/bin/g++.exe")
project(TcpDemo)

add_executable(server server.cpp)
if(WIN32)
    target_link_libraries(server ws2_32)
endif()

add_executable(client client.cpp)
if(WIN32)
    target_link_libraries(client ws2_32)
endif()