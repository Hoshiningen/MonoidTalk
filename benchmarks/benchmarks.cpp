#include "bakery.h"
#include "queries.h"

#include <benchmark/benchmark.h>

#include <numeric>

static bakery::Database g_database{ 1'000'000 };
static const std::array<std::span<const bakery::Transaction>, 5> spans = []()
{
    const auto& transactions = g_database.GetTransactions();
    return std::array<std::span<const bakery::Transaction>, 5>{
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 4) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 1'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 10'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 100'000) },
        //std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 1'000'000) },
        //std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 10'000'000) },
        std::span{ transactions.cbegin(), transactions.cend() }
    };
}();

template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LeastAndGreatestBM(benchmark::State& state)
{
    Derived child{ g_database, spans.at(state.range(0)) };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(query.GetGreatestAndLeastPopularItems());
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::MapReduceParallel)
    ->DenseRange(0, 4)->ArgName("Span")
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::Sequential)
    ->DenseRange(0, 4)->ArgName("Span")
    ->Unit(benchmark::TimeUnit::kMillisecond);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LargestNumberOfPurchasesBM(benchmark::State& state)
{
    Derived child{ g_database, spans.at(state.range(0)) };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(query.GetLargestNumberOfPurachasesMade());
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::MapReduceParallel)
    ->DenseRange(0, 4)->ArgName("Span")
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::Sequential)
    ->DenseRange(0, 4)->ArgName("Span")
    ->Unit(benchmark::TimeUnit::kMillisecond);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void NumberOfTransactionsOver15BM(benchmark::State& state)
{
    Derived child{ g_database, spans.at(state.range(0)) };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(query.GetNumberOfTransactionsOver15());
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::MapReduceParallel)
    ->DenseRange(0, 4)->ArgName("Span")
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::Sequential)
    ->DenseRange(0, 4)->ArgName("Span")
    ->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_MAIN();
