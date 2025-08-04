cmake_minimum_required(VERSION 3.10)

project(ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ChatServer ChatServer/ChatServer.cpp)

target_link_libraries(ChatServer ws2_32)