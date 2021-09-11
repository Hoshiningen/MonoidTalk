#pragma once

#include <bitset>
#include <compare>
#include <filesystem>
#include <random>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ThreadPool;

namespace bakery
{
namespace detail
{
class Random
{
public:
    Random() : engine(std::random_device{}()) {}
    explicit Random(std::mt19937::result_type seed) : engine(seed) {}

    Random(const Random&) = delete;
    Random& operator=(const Random&) = delete;
    
    Random(Random&&) = delete;
    Random& operator=(Random&&) = delete;

    double Value(double min, double max) {
        return std::uniform_real_distribution<>{min, max}(engine);
    }

    int Value(int min, int max) {
        return std::uniform_int_distribution<>{min, max}(engine);
    }

    bool Roll(double chance) {
        return std::bernoulli_distribution{ chance }(engine);
    }

private:
    std::mt19937 engine;
};
} // end namespace detail

enum class FoodType : char
{
    eNone = -1,
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
};

struct Transaction
{
    int orderNumber = 0;
    double gratuity = 0.0;
    std::bitset<27> purchases;

    std::vector<int> GetPurchases() const;

    auto operator<=>(const Transaction&) const = default;
};

struct PurchaseMapping
{
    int foodID = 0;
    int orderNumber = 0;

    auto operator<=>(const PurchaseMapping&) const = default;
};

template<typename DBItem>
using Hashtable = std::unordered_map<int, DBItem>;

template<typename DBItem>
using MultiHashtable = std::unordered_multimap<int, DBItem>;

std::vector<Transaction> GenerateTransactionsSequential(std::size_t amount);
std::vector<Transaction> GenerateTransactionsParallel(std::size_t amount);
MultiHashtable<PurchaseMapping> GeneratePurchaseMapping(const std::vector<Transaction>& transactions);

class Database
{
public:
    Database();
    explicit Database(std::size_t amount);

    Database(std::size_t amount, bool parallelCreation);

    auto operator<=>(const Database&) const = default;

    void Save(const std::filesystem::path& directory) const;
    bool Load(const std::filesystem::path& directory);

    bool CleanDisk(const std::filesystem::path& directory) const;

    const FoodItem& GetFood(int ID) const { return m_foods.at(ID); }
    const Hashtable<FoodItem>& GetFoods() const { return m_foods; }

    const std::vector<Transaction>& GetTransactions() const { return m_transactions; }

    std::span<const Transaction> GetTransactions(std::size_t count) const
    {
        if (count > m_transactions.size())
            return {};

        return { std::cbegin(m_transactions), std::next(std::cbegin(m_transactions), count) };
    }

    std::size_t Size() const { return m_transactions.size(); }

private:
    const Hashtable<FoodItem>& m_foods;
    std::vector<Transaction> m_transactions;
};
}