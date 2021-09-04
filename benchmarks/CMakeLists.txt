add_executable(monoid-benchmarks benchmarks.cpp)

target_include_directories(monoid-benchmarks PRIVATE ${SOURCE_DIR})

target_link_libraries(monoid-benchmarks PRIVATE bakery)
LinkGBenchmark(monoid-benchmarks PRIVATE v1.5.5)

include(${CMAKE_DIR}/LinkThreadPool.cmake)
LinkThreadPool(monoid-benchmarks PRIVATE master)

set_target_properties(monoid-benchmarks PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(monoid-benchmarks PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)