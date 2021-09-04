#include "bakery.h"
#include "queries.h"

#include <benchmark/benchmark.h>

#include <numeric>

std::size_t CountTotalPurchases(const bakery::Database& database)
{
    const auto Transform = [](const auto& pair) {
        return pair.second.purchases.size();
    };

    return std::transform_reduce(std::cbegin(database.GetTransactions()),
        std::cend(database.GetTransactions()), 0ULL, std::plus{}, Transform);
}

//static bakery::Database db1K{ 1'000 };
//static bakery::Database db10K{ 10'000 };
//static bakery::Database db100K{ 100'000 };
static bakery::Database db1M{ 1'000'000 };
//static bakery::Database db10M{ 10'000'000 };
//static bakery::Database db100M{ 100'000'000 };

template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LeastAndGreatestBM(benchmark::State& state)
{
    const int count = std::pow(10, static_cast<int>(state.range(0)));
    Derived child{ db1M, count };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(query.GetGreatestAndLeastPopularItems());
    }
}

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::MapReduceParallel)
    ->DenseRange(3, 6)
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::Sequential)
    ->DenseRange(3, 6)
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_MAIN();
