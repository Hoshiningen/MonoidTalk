#include "bakery.h"
#include "queries.h"

#include <benchmark/benchmark.h>

#include <numeric>

//#define BENCHMARK_TRANSACTION_CREATION
#ifdef BENCHMARK_TRANSACTION_CREATION
static void ParallelTransactionCreationBM(benchmark::State& state)
{
    const std::size_t amount = std::pow(10, state.range(0));

    for (auto _ : state)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(bakery::GenerateTransactionsParallel(amount));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    const std::size_t numItems = state.iterations() * amount;
    state.SetItemsProcessed(numItems);
    state.SetBytesProcessed(numItems * sizeof(bakery::Transaction));
}

static void SequentialTransactionCreationBM(benchmark::State& state)
{
    const std::size_t amount = std::pow(10, state.range(0));

    for (auto _ : state)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(bakery::GenerateTransactionsSequential(amount));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    const std::size_t numItems = state.iterations() * amount;
    state.SetItemsProcessed(numItems);
    state.SetBytesProcessed(numItems * sizeof(bakery::Transaction));
}

BENCHMARK(ParallelTransactionCreationBM)
    ->DenseRange(2, 8)->ArgName("Power10")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK(SequentialTransactionCreationBM)
    ->DenseRange(2, 8)->ArgName("Power10")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

#else
static bakery::Database g_database{ 100'000'000, true };
static const std::array<std::span<const bakery::Transaction>, 7> spans = []()
{
    const auto& transactions = g_database.GetTransactions();
    return std::array<std::span<const bakery::Transaction>, 7>{
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 4) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 1'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 10'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 100'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 1'000'000) },
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 10'000'000) },
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
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetGreatestAndLeastPopularItems());
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LargestNumberOfPurchasesBM(benchmark::State& state)
{
    Derived child{ g_database, spans.at(state.range(0)) };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetLargestNumberOfPurachasesMade());
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());       
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void NumberOfTransactionsOver15BM(benchmark::State& state)
{
    Derived child{ g_database, spans.at(state.range(0)) };
    queries::QueryStrategies& query = child;

    for (auto _ : state)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetNumberOfTransactionsOver15());
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    state.SetItemsProcessed(state.iterations() * spans.at(state.range(0)).size());
}

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

#endif

BENCHMARK_MAIN();
