#pragma once

#include "bakery.h"
#include "ThreadPool.h"

#include <iterator>
#include <span>
#include <stdexcept>

namespace queries
{
namespace detail
{
/// <summary>
/// This chunks up the given span into a series of subspans. It does not require the size of the
/// given span to be a multiple of numChunks, and handles distributing the remainders amongst the subspans.
/// </summary>
template <typename T>
void Chunk(const std::span<T>& span, std::size_t numChunks, std::vector<std::span<T>>& subspans)
{
    if (numChunks > span.size())
        throw std::invalid_argument{ "Can't evenly chunk the container." };

    subspans.clear();

    // Determine the amount of work to do on each thread
    const std::ptrdiff_t chunkSize = span.size() / numChunks;
    int extras = span.size() % numChunks;

    // Determine the amount of items in each chunk first...
    std::vector<std::size_t> counts;
    for (auto i = 0; i < numChunks; ++i, --extras)
    {
        const std::size_t remainder = extras > 0 ? 1 : 0;
        counts.push_back(chunkSize + remainder);
    }

    // Now, iterate over those chunks in reverse, then compute what the offset should be
    std::size_t sizePool = span.size();
    std::for_each(counts.crbegin(), counts.crend(), [&](std::size_t count)
    {
        const std::size_t offset = sizePool - count;
        subspans.insert(subspans.cbegin(), span.subspan(offset, count));

        sizePool -= count;
    });
}

/// <summary>
/// This is a map-reduce function. It's intended to be thrown on a thread in a type-erased lambda, and is
/// used in the map-reduce parallel queries.
/// 
/// The idea behind this method is that it's not necessary to first map all the types to monoids prior
/// to reduction, due to incremental aggregation and the identity property of monoids. This allows me
/// to map and reduce at the same time, yielding O(1) space.
/// </summary>
template<typename T, typename Mapper, typename Reducer, typename Monoid = std::invoke_result_t<Mapper, T>>
    requires std::invocable<Mapper, T> &&
             std::invocable<Reducer, Monoid, Monoid>
Monoid MapReduce(const std::span<const T>& span, Mapper map, Reducer reducer)
{
    Monoid aggregate{};

    for (const auto& value : span)
        aggregate = reducer(aggregate, map(value));

    return aggregate;
}

/// <summary>
/// This is a helper type for incremental aggregation queries. It stores the span and the accumulated
/// result, which can later be pulled out and combined with new monoid reductions.
/// </summary>
template<typename Monoid>
struct CacheEntry
{
    CacheEntry(std::span<const bakery::Transaction> span, const Monoid& aggregate)
        : span(span), aggregate(aggregate)
    {}

    std::span<const bakery::Transaction> span;
    Monoid aggregate;
};
} // end detail namespace

using MinMaxFood = std::pair<bakery::FoodType, bakery::FoodType>;

class QueryStrategies
{
public:
    QueryStrategies(const bakery::Database& database)
        : m_database(database)
    {}

    virtual ~QueryStrategies() = default;

    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span, std::size_t chunkSize) { return {}; }  
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span) = 0;
    virtual std::size_t GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span) = 0;
    virtual std::size_t GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span) = 0;

protected:
    const bakery::Database& m_database;
};

class Sequential : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span) override;
};

class SequentialIA : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span) override;

private:

    std::optional<detail::CacheEntry<std::array<int, 6>>> m_query1Cache;
    std::optional<detail::CacheEntry<std::size_t>> m_query2Cache;
    std::optional<detail::CacheEntry<std::size_t>> m_query3Cache;
};

class MapReduceParallel : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span, std::size_t chunkSize) override;
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span) override;

private:

    ThreadPool m_pool;
};

class MapReduceParallelStd : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span) override;
    virtual std::size_t GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span) override;

private:

    ThreadPool m_pool;
};
} // end queries namespace
