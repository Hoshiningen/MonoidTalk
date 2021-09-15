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

static const bakery::Database g_database{ 100'000'000, true };
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

        // This span has a weird end range due to the randomly increased number of transactions
        // in the incremental aggregation benchmarks. Give an 8M element size buffer, because:
        // Iterations = 1M and new elements / iteration = O(8).
        std::span{ transactions.cbegin(), std::next(transactions.cbegin(), 92'000'000) }
    };
}();

template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LeastAndGreatestBM(benchmark::State& state)
{
    Derived child{ g_database };
    queries::QueryStrategies& query = child;

    bakery::detail::Random random;
    std::size_t numTransactions = 0;

    const auto fullSpan = std::span<const bakery::Transaction>(g_database.GetTransactions());
    const auto& currentSpan = spans.at(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        numTransactions += random.Value(4, 8);
        const auto span = fullSpan.subspan(0, currentSpan.size() + numTransactions);

        state.ResumeTiming();

        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetGreatestAndLeastPopularItems(span));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    state.SetItemsProcessed(state.iterations() * currentSpan.size());
    state.counters["sample_size"] = currentSpan.size();
}

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

// The incremental aggregation queries are throttled by the automatic iteration count feature
// of google benchmark. Once the sample size increases dramatically, to the point where a single
// iteration takes all of the time allotted by google benchmark, then only one iteration will run.
// This severely hinders the performance of incremental aggregation, as the first run is always
// the slowest, and all successive are nearly O(1) speed. Thus, manually specify the number of
// iterations to obtain a more reasonable approximation of the performance of larger sample sizes.
BENCHMARK_TEMPLATE(LeastAndGreatestBM, queries::SequentialIA)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond)
    ->Iterations(1000000);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void LargestNumberOfPurchasesBM(benchmark::State& state)
{
    Derived child{ g_database };
    queries::QueryStrategies& query = child;

    bakery::detail::Random random;
    std::size_t numTransactions = 0;

    const auto fullSpan = std::span<const bakery::Transaction>(g_database.GetTransactions());
    const auto& currentSpan = spans.at(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        numTransactions += random.Value(4, 8);
        const auto span = fullSpan.subspan(0, currentSpan.size() + numTransactions);

        state.ResumeTiming();

        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetLargestNumberOfPurachasesMade(span));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());       
    }

    state.SetItemsProcessed(state.iterations() * currentSpan.size());
    state.counters["sample_size"] = currentSpan.size();
}

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(LargestNumberOfPurchasesBM, queries::SequentialIA)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond)
    ->Iterations(1000000);



template<typename Derived>
    requires std::derived_from< Derived, queries::QueryStrategies>
void NumberOfTransactionsOver15BM(benchmark::State& state)
{
    Derived child{ g_database };
    queries::QueryStrategies& query = child;

    bakery::detail::Random random;
    std::size_t numTransactions = 0;

    const auto fullSpan = std::span<const bakery::Transaction>(g_database.GetTransactions());
    const auto& currentSpan = spans.at(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        numTransactions += random.Value(4, 8);
        const auto span = fullSpan.subspan(0, currentSpan.size() + numTransactions);

        state.ResumeTiming();
        const auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(query.GetNumberOfTransactionsOver15(span));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        state.SetIterationTime(elapsed.count());
    }

    state.SetItemsProcessed(state.iterations() * currentSpan.size());
    state.counters["sample_size"] = currentSpan.size();
}

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::MapReduceParallel)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::Sequential)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond);

BENCHMARK_TEMPLATE(NumberOfTransactionsOver15BM, queries::SequentialIA)
    ->DenseRange(0, 6)->ArgName("Span")
    ->UseManualTime()->Unit(benchmark::TimeUnit::kMillisecond)
    ->Iterations(1000000);
#endif

BENCHMARK_MAIN();
