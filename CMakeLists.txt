cmake_minimum_required(VERSION 3.17)
project(ex2_os)

set(CMAKE_CXX_STANDARD 11)

add_executable(ex2_os main.cpp uthreads.h uthreads.cpp sync_handler.cpp sync_handler.h Thread.cpp Thread.h)
