#include "queries.h"

#include <numeric>
#include <ranges>

namespace queries
{
MinMaxFood Sequential::GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span)
{
    std::array<int, 6> counts{};
    for (const auto& transaction : span)
    {
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            ++counts.at(static_cast<std::size_t>(food.type));
        }
    }

    const auto& [min, max] = std::ranges::minmax_element(counts);

    return {static_cast<bakery::FoodType>(std::distance(std::begin(counts), min)),
            static_cast<bakery::FoodType>(std::distance(std::begin(counts), max))};
}

std::size_t Sequential::GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span)
{
    std::size_t count = 0;
    for (const auto& transaction : span)
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

std::size_t Sequential::GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span)
{
    std::size_t maxPurchases = 0;
    for (const auto& transaction : span)
        maxPurchases = std::max(maxPurchases, transaction.GetPurchases().size());

    return maxPurchases;
}



MinMaxFood SequentialIA::GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span)
{
    const auto CountFoodTypes = [this](const std::span<const bakery::Transaction>& span, const auto& prev)
    {
        std::array<int, 6> counts = prev;
        for (const auto& transaction : span)
        {
            for (int foodID : transaction.GetPurchases())
            {
                const bakery::FoodItem& food = m_database.GetFood(foodID);
                ++counts.at(static_cast<std::size_t>(food.type));
            }
        }

        return counts;
    };

    if (m_query1Cache)
    {
        const auto deltaSpan = span.subspan(m_query1Cache->span.size());

        m_query1Cache->aggregate = CountFoodTypes(deltaSpan, m_query1Cache->aggregate);
        m_query1Cache->span = span;
    }
    else
    {
        m_query1Cache.emplace(span, CountFoodTypes(span, std::array<int, 6>{}));
    }

    const auto& [min, max] = std::ranges::minmax_element(m_query1Cache->aggregate);

    return {static_cast<bakery::FoodType>(std::distance(std::begin(m_query1Cache->aggregate), min)),
            static_cast<bakery::FoodType>(std::distance(std::begin(m_query1Cache->aggregate), max))};
}

std::size_t SequentialIA::GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span)
{
    const auto GetNumTransactionsOver15 = [this](const std::span<const bakery::Transaction>& span, std::size_t prev)
    {
        std::size_t count = prev;
        for (const auto& transaction : span)
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
    };

    if (m_query2Cache)
    {
        const auto deltaSpan = span.subspan(m_query2Cache->span.size());

        m_query2Cache->aggregate = GetNumTransactionsOver15(deltaSpan, m_query2Cache->aggregate);
        m_query2Cache->span = span;
    }
    else
    {
        m_query2Cache.emplace(span, GetNumTransactionsOver15(span, 0));
    }

    return m_query2Cache->aggregate;
}

std::size_t SequentialIA::GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span)
{
    const auto GetMaxPurchasesMade = [](const std::span<const bakery::Transaction>& span, std::size_t prev)
    {
        std::size_t maxPurchases = prev;
        for (const auto& transaction : span)
            maxPurchases = std::max(maxPurchases, transaction.GetPurchases().size());

        return maxPurchases;
    };

    if (m_query3Cache)
    {
        const auto deltaSpan = span.subspan(m_query3Cache->span.size());

        m_query3Cache->aggregate = GetMaxPurchasesMade(deltaSpan, m_query3Cache->aggregate);
        m_query3Cache->span = span;
    }
    else
    {
        m_query3Cache.emplace(span, GetMaxPurchasesMade(span, 0));
    }

    return m_query3Cache->aggregate;
}



MinMaxFood MapReduceParallel::GetGreatestAndLeastPopularItems(const std::span<const bakery::Transaction>& span)
{
    using Monoid = std::array<int, 6>;
    const auto Map = [this](const auto& transaction)
    {
        Monoid monoid{};
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            ++monoid.at(static_cast<std::size_t>(food.type));
        }

        return monoid;
    };

    const auto Reduce = [](const Monoid& aggregate, const Monoid& next)
    {
        Monoid result = aggregate;
        for (std::size_t index = 0; index < result.size(); ++index)
            result.at(index) += next.at(index);

        return result;
    };

    std::vector<std::span<const bakery::Transaction>> chunks;
    detail::Chunk(span, m_pool.ThreadCount(), chunks);

    std::vector<std::future<Monoid>> futures;
    for (const auto& chunk : chunks)
        futures.push_back(m_pool.Run([&]() { return detail::MapReduce(chunk, Map, Reduce); }));

    Monoid result = std::accumulate(std::begin(futures), std::end(futures), Monoid{},
        [&](const Monoid& aggregate, std::future<Monoid>& next) {
            return Reduce(aggregate, next.get());
        });

    const auto& [min, max] = std::ranges::minmax_element(result);

    return {static_cast<bakery::FoodType>(std::distance(std::begin(result), min)),
            static_cast<bakery::FoodType>(std::distance(std::begin(result), max))};
}

std::size_t MapReduceParallel::GetNumberOfTransactionsOver15(const std::span<const bakery::Transaction>& span)
{
    using Monoid = int;
    const auto Map = [this](const auto& transaction)
    {
        double total = 0.0;
        for (int foodID : transaction.GetPurchases())
        {
            const bakery::FoodItem& food = m_database.GetFood(foodID);
            total += food.cost;
        }

        return total > 15.0 ? 1 : 0;
    };

    std::vector<std::span<const bakery::Transaction>> chunks;
    detail::Chunk(span, m_pool.ThreadCount(), chunks);

    std::vector<std::future<Monoid>> futures;
    for (const auto& chunk : chunks)
        futures.push_back(m_pool.Run([&]() { return detail::MapReduce(chunk, Map, std::plus<int>{}); }));

    return std::accumulate(std::begin(futures), std::end(futures), Monoid{},
        [&](const Monoid& aggregate, std::future<Monoid>& next) {
            return aggregate + next.get();
        });
}

std::size_t MapReduceParallel::GetLargestNumberOfPurachasesMade(const std::span<const bakery::Transaction>& span)
{
    using Monoid = int;
    const auto Map = [this](const auto& transaction) {
        return transaction.GetPurchases().size();
    };

    const auto Reduce = [](const Monoid& aggregate, const Monoid& next) {
        return std::max(aggregate, next);
    };

    std::vector<std::span<const bakery::Transaction>> chunks;
    detail::Chunk(span, m_pool.ThreadCount(), chunks);

    std::vector<std::future<Monoid>> futures;
    for (const auto& chunk : chunks)
        futures.push_back(m_pool.Run([&]() -> Monoid { return detail::MapReduce(chunk, Map, Reduce); }));

    return std::accumulate(std::begin(futures), std::end(futures), std::numeric_limits<Monoid>::min(),
        [&](const Monoid& aggregate, std::future<Monoid>& next) {
            return Reduce(aggregate, next.get());
        });
}
} // end queries namespace
