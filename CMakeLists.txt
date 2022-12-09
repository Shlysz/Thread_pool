cmake_minimum_required(VERSION 3.13)
project(Thread_pool)

set(CMAKE_CXX_STANDARD 14)

add_executable(Thread_pool main.cpp  TaskQueue.cpp TaskQueue.h ThreadPoolWithClass.cpp ThreadPoolWithClass.h)

find_package( Threads REQUIRED )
target_link_libraries(Thread_pool Threads::Threads )
