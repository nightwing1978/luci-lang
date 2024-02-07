cmake_minimum_required (VERSION 3.8)

project ("luci")
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

# Include sub-projects.
add_subdirectory ("explorations")
add_subdirectory ("interp")
