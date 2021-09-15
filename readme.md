# The Power of Monoids

This was created to support a tech talk by the same name, and contains a practical monoid application (performing database queries via map-reduce and incremental aggregation).

There are a series of benchmarks to profile the database queries, and there are unit tests to ensure correct behavior in database operations / queries.

## Building

This is built using C++20 and has a few dependencies that are fetched during configuration. See [the cmake folder](cmake) for a list of them. This has only been tested on `MSVC 19.29.30040.0`.

To build (from the root directory):

```cmake
mkdir build && cd build
cmake ..
cmake --build .
```

## Plotting

A script is provided to plot the benchmark results. It leverages `matplotlib` to handle the chart creation, and thus it's necessary to install the script's dependencies.

Within a virtual environment, or not, at the root directory of this repo, invoke:

```bash
pip install -r requirements.txt
```

Then, it's necessary to generate the benchmark results. Locate the `monoid-benchmarks` executable and invoke:

```bash
./monoid-benchmarks.exe --benchmark_format=json > results.json
```

Now, with the benchmark results in hand, you can plot them by:

```bash
python plot.py results.json
```