#include "tests.h"

#include "bakery.h"

#include <concepts>
#include <filesystem>
#include <ranges>

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
        ASSERT_EQ(trans1.first, trans2.first);
        ASSERT_DOUBLE_EQ(trans1.second.gratuity, trans2.second.gratuity);

        const std::vector<int> purchases1 = trans1.second.GetPurchases();
        const std::vector<int> purchases2 = trans2.second.GetPurchases();

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
    const auto transactions = bakery::GenerateTransactions(7);
    const auto purchaseMapping = bakery::GeneratePurchaseMapping(transactions);;

    // evaluate food purchases
    for (const auto& [orderNumber, transaction] : transactions)
    {
        ASSERT_EQ(transaction.purchases.size(), purchaseMapping.count(orderNumber));

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
