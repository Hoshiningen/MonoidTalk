#include "bakery.h"
#include "queries.h"

#include <concepts>
#include <filesystem>
#include <ranges>

#include <gtest/gtest.h>

class DatabaseTests : public ::testing::Test {};
class QueryTests : public ::testing::Test {};

namespace utility
{
template<std::ranges::input_range Range1, std::ranges::input_range Range2,
    typename Return = std::pair<typename Range1::value_type, typename Range2::value_type>>
std::vector<Return> Zip(const Range1& a, const Range2& b)
{
    std::vector<Return> result;
    std::ranges::transform(a, b, std::back_inserter(result),
        [](const auto& item1, const auto& item2) { return std::make_pair(item1, item2); });

    return result;
}

void CompareDatabaseEquality(const bakery::Database& a, const bakery::Database& b)
{
    const auto& first = a.GetTransactions();
    const auto& second = b.GetTransactions();

    ASSERT_FALSE(first.empty());
    ASSERT_EQ(first.size(), second.size());

    for (const auto& [trans1, trans2] : Zip(first, second))
    {
        ASSERT_EQ(trans1.orderNumber, trans2.orderNumber);
        ASSERT_DOUBLE_EQ(trans1.gratuity, trans2.gratuity);

        const std::vector<int> purchases1 = trans1.GetPurchases();
        const std::vector<int> purchases2 = trans2.GetPurchases();

        ASSERT_FALSE(purchases1.empty());
        ASSERT_EQ(purchases1.size(), purchases2.size());

        for (const auto& [foodID1, foodID2] : Zip(purchases1, purchases2))
            ASSERT_EQ(foodID1, foodID2);
    }
}
}

TEST_F(DatabaseTests, Generation)
{
    // Transactions all have a unique
    const auto transactions = bakery::GenerateTransactionsSequential(7);
    const auto purchaseMapping = bakery::GeneratePurchaseMapping(transactions);

    // evaluate food purchases
    for (const auto& transaction : transactions)
    {
        const auto orderNumber = transaction.orderNumber;
        ASSERT_EQ(transaction.GetPurchases().size(), purchaseMapping.count(orderNumber));

        std::unordered_map<int, int> itemCounts;
        std::unordered_map<int, int> mappingCounts;

        for (int foodID : transaction.GetPurchases())
            itemCounts[foodID]++;

        const auto filter = [&orderNumber](const auto& pair) { return pair.first == orderNumber; };
        for (const auto& [orderNumber, purchase] : purchaseMapping | std::views::filter(filter))
            mappingCounts[purchase.foodID]++;

        ASSERT_EQ(itemCounts, mappingCounts);
    }
}

TEST_F(DatabaseTests, Serialization)
{
    bakery::Database database1{7};
    database1.Save("./");
    
    bakery::Database database2;
    database2.Load("./");
    database1.CleanDisk("./");

    utility::CompareDatabaseEquality(database1, database2);
}

TEST_F(QueryTests, GreatestAndLeastPopularItems)
{
    const bakery::Database database{ 100'000, true };

    queries::MapReduceParallel strat1{ database };
    queries::Sequential strat2{ database };
    queries::SequentialIA strat3{ database };

    const auto [min1, max1] = strat1.GetGreatestAndLeastPopularItems(database.GetTransactions());
    const auto [min2, max2] = strat2.GetGreatestAndLeastPopularItems(database.GetTransactions());
    const auto [min3, max3] = strat3.GetGreatestAndLeastPopularItems(database.GetTransactions());

    ASSERT_TRUE(min1 == min2 && min2 == min3);
    ASSERT_TRUE(max1 == max2 && max2 == max3);
}

TEST_F(QueryTests, NumberOfTransactionsOver15)
{
    const bakery::Database database{ 100'000, true };

    queries::MapReduceParallel strat1{ database };
    queries::Sequential strat2{ database };
    queries::SequentialIA strat3{ database };

    const std::size_t num1 = strat1.GetNumberOfTransactionsOver15(database.GetTransactions());
    const std::size_t num2 = strat2.GetNumberOfTransactionsOver15(database.GetTransactions());
    const std::size_t num3 = strat3.GetNumberOfTransactionsOver15(database.GetTransactions());

    ASSERT_TRUE(num1 == num2 && num2 == num3);
}

TEST_F(QueryTests, LargestNumberOfPurachasesMade)
{
    const bakery::Database database{ 100'000, true };

    queries::MapReduceParallel strat1{ database };
    queries::Sequential strat2{ database };
    queries::SequentialIA strat3{ database };

    const std::size_t count1 = strat1.GetLargestNumberOfPurachasesMade(database.GetTransactions());
    const std::size_t count2 = strat2.GetLargestNumberOfPurachasesMade(database.GetTransactions());
    const std::size_t count3 = strat3.GetLargestNumberOfPurachasesMade(database.GetTransactions());

    ASSERT_TRUE(count1 == count2 && count2 == count3);
}
