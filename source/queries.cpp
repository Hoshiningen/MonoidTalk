#include "queries.h"

#include <execution>
#include <numeric>
#include <ranges>

namespace
{
template<typename Container, typename Op, typename T = Container::value_type>
T LeftFold(const Container& container, T init, Op binaryOp)
{
    return std::accumulate(
        container.cbegin(), 
        container.cend(), 
        init,
        binaryOp
    );
}

template<typename Container, typename Op, typename T = Container::value_type>
T LeftFold2(const Container& container, T init, Op binOp)
{
    return std::reduce(
        std::execution::par,
        container.cbegin(), 
        container.cend(), 
        init,
        binOp);
}

template<typename Container, typename Op, typename T = Container::value_type>
T RightFold(const Container& container, T init, Op binOp)
{
    const auto ReverseParameters = [&](const T& a, const T& b) { return binOp(b, a); };
    return std::accumulate(container.crbegin(), container.crend(), init, ReverseParameters);
}

template<typename Container, typename Op, typename T = Container::value_type>
T RightFold2(const Container& container, T init, Op binOp)
{
    return std::reduce(
        std::execution::par,
        container.crbegin(),
        container.crend(),
        init,
        [&](const T& a, const T& b) { return binOp(b, a); }
    );
}
} //unnamed namespace

namespace queries
{
QueryStrategies::QueryStrategies(const bakery::Database& database, int count)
    : m_database(database)
{
    std::ranges::transform(database.GetTransactions(count), std::back_inserter(m_transactions),
        [](auto pair) { return pair.second; });
}

MinMaxFood Sequential::GetGreatestAndLeastPopularItems()
{
    std::unordered_map<bakery::FoodType, int> counts;
    for (const auto& [orderNumber, transaction] : m_database.GetTransactions())
    {
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            counts[food.type]++;
        }
    }

    const auto Predicate = [](const auto& a, const auto& b) {
        return a.second < b.second;
    };

    const auto& [min, max] = std::ranges::minmax_element(counts, Predicate);
    return { min->first, max->first };
}

std::size_t Sequential::GetNumberOfTransactionsOver15()
{
    std::size_t count = 0;
    for (const auto& [orderNumber, transaction] : m_database.GetTransactions())
    {
        double total = 0.0;
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            total += food.cost;
        }

        if (total > 15.0)
            ++count;
    }

    return count;
}

std::size_t Sequential::GetLargestNumberOfPurachasesMade()
{
    std::size_t maxPurchases = 0;
    for (const auto& [orderNumber, transaction] : m_database.GetTransactions())
        maxPurchases = std::max(maxPurchases, transaction.GetPurchases().size());

    return maxPurchases;
}




MinMaxFood SequentialIA::GetGreatestAndLeastPopularItems()
{
    return MinMaxFood();
}

std::size_t SequentialIA::GetNumberOfTransactionsOver15()
{
    return std::size_t();
}

std::size_t SequentialIA::GetLargestNumberOfPurachasesMade()
{
    return std::size_t();
}


/*
* 1. Run sequential until XXXX items
* 2. Batch up the rest and parallelize
*
* Options for parallelizing this:
* 1. Map everything in parallel first, then reduce in parallel once mapping is done
    - There isn't any more computations happening here
    - O(n + numChunks) space
* 2. Batch up chunks of work to map-reduce on their own threads, then combine results
*   - Same amount of computations
*   - O(2 * numChunks) space
*/


MinMaxFood MapReduceParallel::GetGreatestAndLeastPopularItems()
{
    using Monoid = std::unordered_map<bakery::FoodType, int>;
    const auto Map = [this](const auto& transaction)
    {
        Monoid monoid;
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            ++monoid[food.type];
        }

        return monoid;
    };

    const auto Reduce = [](const Monoid& aggregate, const Monoid& next)
    {
        Monoid result = aggregate;
        for (auto& [type, count] : next)
            result[type] += count;

        return result;
    };

    std::vector<std::span<const bakery::Transaction>> chunks;
    Chunk(m_transactions, m_pool.ThreadCount(), chunks);

    std::vector<std::future<Monoid>> futures;
    for (const auto& chunk : chunks)
        futures.push_back(m_pool.Run([&]() { return MapReduce(chunk, Map, Reduce); }));

    Monoid result = std::accumulate(std::begin(futures), std::end(futures), Monoid{},
        [&](const Monoid& aggregate, std::future<Monoid>& next) {
            return Reduce(aggregate, next.get());
        });

    const auto Predicate = [](const auto& a, const auto& b) {
        return a.second < b.second;
    };

    const auto& [min, max] = std::ranges::minmax_element(result, Predicate);
    return { min->first, max->first };
}

std::size_t MapReduceParallel::GetNumberOfTransactionsOver15()
{
    return std::size_t();
}

std::size_t MapReduceParallel::GetLargestNumberOfPurachasesMade()
{
    return std::size_t();
}




MinMaxFood MapReduceParallelIA::GetGreatestAndLeastPopularItems()
{
    return MinMaxFood();
}

std::size_t MapReduceParallelIA::GetNumberOfTransactionsOver15()
{
    return std::size_t();
}

std::size_t MapReduceParallelIA::GetLargestNumberOfPurachasesMade()
{
    return std::size_t();
}
} // end queries namespace
