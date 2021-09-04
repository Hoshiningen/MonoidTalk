cmake_minimum_required(VERSION 3.21.0 FATAL_ERROR)
project(PowerOfMonoids CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set directory paths
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Pull down all the linking cmake scripts
include(${CMAKE_DIR}/FetchCMakeUtilities.cmake)
FetchCMakeUtilities()

include(${cmake_utility_SOURCE_DIR}/cmake/google/LinkGBenchmark.cmake)
include(${cmake_utility_SOURCE_DIR}/cmake/google/LinkGTest.cmake)

add_executable(monoid-app ${SOURCE_DIR}/main.cpp)

target_link_libraries(monoid-app PRIVATE bakery)

include(${CMAKE_DIR}/LinkThreadPool.cmake)
LinkThreadPool(monoid-app PRIVATE master)

set_target_properties(monoid-app PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(monoid-app PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

add_subdirectory(source)
add_subdirectory(tests)
add_subdirectory(benchmarks)