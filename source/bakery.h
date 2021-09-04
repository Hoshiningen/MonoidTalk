#pragma once

#include <bitset>
#include <compare>
#include <filesystem>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace bakery
{
enum class FoodType
{
    eNone,
    eBagel,
    eBread,
    eCookie,
    ePastry,
    eBeverage,
    eSandwich
};

struct FoodItem
{
    int foodID = 0;
    std::string name;
    FoodType type = FoodType::eNone;
    double cost = 0.0;

    auto operator<=>(const FoodItem&) const = default;
    std::size_t operator()(const FoodItem&) const;
};

struct Transaction
{
    int orderNumber = 0;
    double gratuity = 0.0;
    std::bitset<27> purchases;

    std::vector<int> GetPurchases() const;

    auto operator<=>(const Transaction&) const = default;
    std::size_t operator()(const Transaction& item) const;
};

struct PurchaseMapping
{
    int foodID = 0;
    int orderNumber = 0;

    auto operator<=>(const PurchaseMapping&) const = default;
    std::size_t operator()(const PurchaseMapping& item) const;
};

template<typename DBItem>
using Hashtable = std::unordered_map<int, DBItem>;

template<typename DBItem>
using MultiHashtable = std::unordered_multimap<int, DBItem>;

Hashtable<Transaction> GenerateTransactions(std::size_t amount);
MultiHashtable<PurchaseMapping> GeneratePurchaseMapping(const Hashtable<Transaction>& transactions);

class Database
{
public:
    Database();
    explicit Database(std::size_t amount);

    auto operator<=>(const Database&) const = default;

    void Save(const std::filesystem::path& directory) const;
    bool Load(const std::filesystem::path& directory);

    bool CleanDisk(const std::filesystem::path& directory) const;

    const FoodItem& GetFood(int ID) const { return m_foods.at(ID); }
    const Hashtable<FoodItem>& GetFoods() const { return m_foods; }

    const Hashtable<Transaction>& GetTransactions() const { return m_transactions; }
    auto GetTransactions(int count) const { return std::ranges::take_view(m_transactions, count); }

private:
    const Hashtable<FoodItem>& m_foods;
    Hashtable<Transaction> m_transactions;
};
}