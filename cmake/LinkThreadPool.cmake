include(FetchContent)

macro(LinkThreadPool TARGET ACCESS TAG)
    FetchContent_Declare(
        threadpool
        GIT_REPOSITORY https://github.com/Hoshiningen/ThreadPool.git
        GIT_TAG ${TAG}
    )

    FetchContent_GetProperties(threadpool)

    if (NOT threadpool_POPULATED)
        FetchContent_Populate(threadpool)

        set(BUILD_BENCHMARKS OFF CACHE BOOL "Build Benchmarks" FORCE)
        set(BUILD_APP OFF CACHE BOOL "Build Benchmarks" FORCE)

        add_subdirectory(${threadpool_SOURCE_DIR}/include ${threadpool_BINARY_DIR})
    endif()

    target_link_libraries(${TARGET} ${ACCESS} threadpool-lib)
    target_include_directories(${TARGET} ${ACCESS} ${threadpool_SOURCE_DIR}/include)
endmacro()