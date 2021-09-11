add_executable(monoid-tests tests.cpp)

target_include_directories(monoid-tests PRIVATE ${SOURCE_DIR})

target_link_libraries(monoid-tests PRIVATE bakery)
LinkGTest(monoid-tests PRIVATE release-1.11.0)

include(${CMAKE_DIR}/LinkThreadPool.cmake)
LinkThreadPool(monoid-tests PRIVATE master)

set_target_properties(monoid-tests PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(monoid-tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)