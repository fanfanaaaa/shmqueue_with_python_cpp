cmake_minimum_required(VERSION 3.6)
project(shmqueue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11")

# 编译生成 libshmmqueue.so 动态库
add_library(shmmqueue SHARED shmmqueueWapper.c shmmqueueWapper.h shm_rwlockWapper.h shm_rwlockWapper.c)
