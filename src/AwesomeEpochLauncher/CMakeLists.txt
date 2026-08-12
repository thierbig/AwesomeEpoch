cmake_minimum_required(VERSION 3.15)
project(AwesomeEpochLauncher)

enable_language(RC)

# The root scope adds cl-only options (/W3, /O2, /Gw, /GL) via add_compile_options, and the
# directory property feeds every language - rc.exe rejects them. Drop the inherited list here;
# CMAKE_CXX_FLAGS_<CONFIG> still carries the optimisation flags for this small launcher.
set_directory_properties(PROPERTIES COMPILE_OPTIONS "")

add_executable(AwesomeEpochLauncher Main.cpp AwesomeEpoch.rc)

# Ship as AwesomeEpoch.exe (the launcher the README/users expect).
set_target_properties(AwesomeEpochLauncher PROPERTIES OUTPUT_NAME "AwesomeEpoch")

target_link_libraries(AwesomeEpochLauncher PRIVATE shell32)
