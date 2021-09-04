add_library(bakery
    bakery.h
    bakery.cpp
    queries.h
    queries.cpp)

set_target_properties(bakery PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(bakery PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

include(${CMAKE_DIR}/LinkThreadPool.cmake)
LinkThreadPool(bakery PRIVATE master)