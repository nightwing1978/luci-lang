cmake_minimum_required (VERSION 3.8)

# c++20 is needed for the std::ranges::view in Evaluator

project ("luci")
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
endif(MSVC)

# Include sub-projects.
add_subdirectory ("explorations")
add_subdirectory ("interp")
