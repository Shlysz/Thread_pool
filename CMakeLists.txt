cmake_minimum_required(VERSION 3.13)
project(Thread_pool)

set(CMAKE_CXX_STANDARD 14)

add_executable(Thread_pool main.cpp ThreadPool.h ThreadPool.cpp)
