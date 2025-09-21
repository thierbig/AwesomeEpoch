cmake_minimum_required(VERSION 3.16)

project(AwesomeWotlkInjector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AwesomeWotlkInjector 
    Injector.cpp
    ManualMapper.h
    ManualMapper.cpp)

# The injector needs to be linked against kernel32 for the Windows API calls,
# but this is usually handled automatically by the MSVC compiler toolchain.
# If you were using MinGW or another compiler, you might need:
# target_link_libraries(AwesomeWotlkInjector PRIVATE kernel32)
