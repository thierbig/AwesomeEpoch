cmake_minimum_required(VERSION 3.15)
project(AwesomeEpochLauncher)

add_executable(AwesomeEpochLauncher Main.cpp)

# Ship as AwesomeEpoch.exe (the launcher the README/users expect).
set_target_properties(AwesomeEpochLauncher PROPERTIES OUTPUT_NAME "AwesomeEpoch")

target_link_libraries(AwesomeEpochLauncher PRIVATE shell32)
