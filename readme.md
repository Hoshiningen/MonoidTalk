# The Power of Monoids

This was created to support a tech talk by the same name, and contains a practical monoid application (performing database queries via map-reduce and incremental aggregation).

There are a series of benchmarks to profile the database queries, and there are unit tests to ensure correct behavior in database operations / queries.

## Building

This is built using C++20 and has a few dependencies that are fetched during configuration. See [the cmake folder](cmake) for a list of them. This has only been tested on `MSVC 16.10.4`.

To build (from the root directory):

```cmake
mkdir build && cd build
cmake ..
cmake --build .
```