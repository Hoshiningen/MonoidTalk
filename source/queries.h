#pragma once

#include "bakery.h"
#include "ThreadPool.h"

#include <iterator>
#include <span>
#include <stdexcept>

namespace queries
{
using MinMaxFood = std::pair<bakery::FoodType, bakery::FoodType>;

template <typename Container, typename Value = typename Container::value_type>
    requires std::contiguous_iterator<typename Container::const_iterator>
void Chunk(const Container& container, std::size_t numChunks, std::vector<std::span<const Value>>& spans)
{
    // This is pretty lame, but I don't want to spend the time to get this working right.
    if (numChunks > container.size() || container.size() % numChunks != 0)
        throw std::invalid_argument{ "Can't evenly chunk the container." };

    spans.clear();

    // Determine the amount of work to do on each thread
    const std::ptrdiff_t chunkSize = container.size() / numChunks;
    for (auto i = 1; i <= numChunks; ++i)
    {
        spans.emplace_back(
            std::next(container.cbegin(), (i - 1) * chunkSize),
            std::next(container.cbegin(), i * chunkSize));
    }
}

template<typename T, typename Mapper, typename Reducer, typename Monoid = std::invoke_result_t<Mapper, T>>
    requires std::invocable<Mapper, T> &&
             std::invocable<Reducer, Monoid, Monoid>
Monoid MapReduce(const std::span<const T>& span, Mapper map, Reducer reducer)
{
    Monoid aggregate;

    for (const auto& value : span)
        aggregate = reducer(aggregate, map(value));

    return aggregate;
}

class QueryStrategies
{
public:
    QueryStrategies(const bakery::Database& database, int count);
    virtual ~QueryStrategies() = default;

    virtual MinMaxFood GetGreatestAndLeastPopularItems() = 0;
    virtual std::size_t GetNumberOfTransactionsOver15() = 0;
    virtual std::size_t GetLargestNumberOfPurachasesMade() = 0;

protected:

    const bakery::Database& m_database;
    std::vector<bakery::Transaction> m_transactions;
};

class Sequential : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems() override;
    virtual std::size_t GetNumberOfTransactionsOver15() override;
    virtual std::size_t GetLargestNumberOfPurachasesMade() override;
};

class SequentialIA : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems() override;
    virtual std::size_t GetNumberOfTransactionsOver15() override;
    virtual std::size_t GetLargestNumberOfPurachasesMade() override;
};

class MapReduceParallel : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems() override;
    virtual std::size_t GetNumberOfTransactionsOver15() override;
    virtual std::size_t GetLargestNumberOfPurachasesMade() override;

private:

    ThreadPool m_pool;
};

class MapReduceParallelIA : public QueryStrategies
{
public:
    using QueryStrategies::QueryStrategies;

    // Inherited via QueryStrategies
    virtual MinMaxFood GetGreatestAndLeastPopularItems() override;
    virtual std::size_t GetNumberOfTransactionsOver15() override;
    virtual std::size_t GetLargestNumberOfPurachasesMade() override;

private:

    ThreadPool m_pool;
};
} // end queries namespace
