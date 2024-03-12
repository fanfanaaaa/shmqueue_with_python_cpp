cmake_minimum_required(VERSION 3.6)
project(shmqueue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11 -g")

set(SOURCE_FILES write.cpp
        ../shmmqueue.cpp
        ../shmmqueue.h
        ../shm_rwlock.h
        ../shm_rwlock.cpp)
add_executable(write ${SOURCE_FILES})
set(SOURCE_FILES read.cpp
        ../shmmqueue.cpp
        ../shmmqueue.h
        ../shm_rwlock.h
        ../shm_rwlock.cpp)
add_executable(read ${SOURCE_FILES})