#include "bakery.h"

#include <array>
#include <execution>
#include <fstream>
#include <filesystem>
#include <future>
#include <iterator>
#include <random>
#include <ranges>
#include <thread>
#include <utility>

#define SeededRandom

namespace settings
{
// gratuity
constexpr double kMinGratuity = 0.1;
constexpr double kMaxGratuity = 0.35;
constexpr double kGratutityChance = 0.95;

// purchase settings
constexpr double kBeverageChance = 0.5;
constexpr double kCookieChance = 0.42;
constexpr double kBreakfastChance = 0.777;
constexpr double kBagelChance = 0.65;
constexpr double kLoafChance = 0.15;
}

namespace
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

int Select(bakery::FoodType type, const bakery::Hashtable<bakery::FoodItem>& foods, Random& random)
{
    const std::size_t count = std::ranges::count_if(foods,
        [&type](const auto& pair) { return pair.second.type == type; });

    const int relativeIdx = random.Value(0, count - 1);

    const auto foundIter = std::ranges::find_if(foods, [index = 0, &type, &relativeIdx](const auto& pair) mutable {
        return pair.second.type == type && index++ == relativeIdx;
    });

    if (foundIter == std::cend(foods))
        throw std::logic_error("Couldn't find a food item with that index.");

    return foundIter->second.foodID;
}

std::bitset<27> GenerateTicket(const bakery::Hashtable<bakery::FoodItem>& foods, Random& random)
{
    std::bitset<27> items;
    if (foods.empty())
        return items;

    if (random.Roll(settings::kBeverageChance))
        items.set(Select(bakery::FoodType::eBeverage, foods, random));

    if (random.Roll(settings::kLoafChance))
        items.set(Select(bakery::FoodType::eBread, foods, random));

    if (random.Roll(settings::kBreakfastChance))
    {
        if (random.Roll(settings::kBagelChance))
            items.set(Select(bakery::FoodType::eBagel, foods, random));
        else
            items.set(Select(bakery::FoodType::ePastry, foods, random));
    }
    else // Lunch items
    {
        if (random.Roll(settings::kCookieChance))
            items.set(Select(bakery::FoodType::eCookie, foods, random));

        items.set(Select(bakery::FoodType::eSandwich, foods, random));
    }

    return items;
}

double GenerateGratuity(Random& random)
{
    double gratuity = 0.0;

    if (random.Roll(settings::kGratutityChance))
        gratuity = random.Value(settings::kMinGratuity, settings::kMaxGratuity);

    return gratuity;
}

bool Equals(double a, double b, double epsilon = 1e-5)
{
    return std::fabs(a - b) < epsilon;
}
} // end unnamed namespace

namespace bakery
{
std::ostream& operator<<(std::ostream& stream, const bakery::FoodItem& item)
{
    stream << item.foodID << ",";
    stream << item.name << ",";
    stream << std::setprecision(21) << item.cost << ",";
    stream << static_cast<int>(item.type) << "\n";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const bakery::Transaction& item)
{
    stream << item.orderNumber << ",";
    stream << std::setprecision(21) << item.gratuity << "\n";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const bakery::PurchaseMapping& item)
{
    stream << item.orderNumber << ",";
    stream << item.foodID << "\n";

    return stream;
}

std::istream& operator>>(std::istream& stream, bakery::FoodItem& item)
{
    char comma = '0';
    stream >> item.foodID >> comma;

    std::array<char, 100> name;
    stream.getline(name.data(), name.size(), ',');
    item.name = name.data();

    stream >> item.cost >> comma;

    int temp = 0;
    stream >> temp;

    item.type = static_cast<bakery::FoodType>(temp);

    return stream;
}

std::istream& operator>>(std::istream& stream, bakery::Transaction& item)
{
    char comma = '0';
    stream >> item.orderNumber >> comma;
    stream >> item.gratuity;

    return stream;
}

std::istream& operator>>(std::istream& stream, bakery::PurchaseMapping& item)
{
    char comma = '0';
    stream >> item.orderNumber >> comma;
    stream >> item.foodID;

    return stream;
}

std::size_t FoodItem::operator()(const FoodItem& item) const
{
    return std::hash<int>{}(item.foodID);
}

std::size_t Transaction::operator()(const Transaction& item) const
{
    return std::hash<int>{}(item.orderNumber);
}

std::size_t PurchaseMapping::operator()(const PurchaseMapping& item) const
{
    const std::size_t hash1 = std::hash<int>{}(item.foodID);
    const std::size_t hash2 = std::hash<int>{}(item.orderNumber);
    
    return hash1 ^ (hash2 << 1);
}

const Hashtable<FoodItem>& GenerateFoods()
{
    static const Hashtable<FoodItem> foods =
    {
        { 0, {.foodID = 0, .name = "Everything Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 1, {.foodID = 1, .name = "Plain Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 2, {.foodID = 2, .name = "Asiago Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 3, {.foodID = 3, .name = "Rosemary Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 4, {.foodID = 4, .name = "Tomato Thyme Bagel", .type = FoodType::eBagel, .cost = 1.70 }},
        { 5, {.foodID = 5, .name = "Green Tea Bagel", .type = FoodType::eBagel, .cost = 1.60 }},
        { 6, {.foodID = 6, .name = "Roasted Pepper Bagel", .type = FoodType::eBagel, .cost = 1.70 }},
        { 7, {.foodID = 7, .name = "Sesame Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 7, {.foodID = 7, .name = "Onion Bagel", .type = FoodType::eBagel, .cost = 1.50 }},
        { 8, {.foodID = 8, .name = "Spinach Parmesan Bagel", .type = FoodType::eBagel, .cost = 1.70 }},
        { 9, {.foodID = 9, .name = "Spinach Pesto Bagel", .type = FoodType::eBagel, .cost = 1.70 }},
        {10, {.foodID = 10, .name = "White Bread", .type = FoodType::eBread, .cost = 4.99 }},
        {11, {.foodID = 11, .name = "Pumpernickel Bread", .type = FoodType::eBread, .cost = 4.99 }},
        {12, {.foodID = 12, .name = "Everything Bread", .type = FoodType::eBread, .cost = 4.99 }},
        {13, {.foodID = 13, .name = "Rosemary Bread", .type = FoodType::eBread, .cost = 4.99 }},
        {14, {.foodID = 14, .name = "Cinnamon Roll", .type = FoodType::ePastry, .cost = 1.70 }},
        {15, {.foodID = 15, .name = "Cranberry Walnut Sticky Bun", .type = FoodType::ePastry, .cost = 1.70 }},
        {16, {.foodID = 16, .name = "Blueberry Hand Pie", .type = FoodType::ePastry, .cost = 1.70 }},
        {17, {.foodID = 17, .name = "Grilled Cheese", .type = FoodType::eSandwich, .cost = 2.00 }},
        {18, {.foodID = 18, .name = "Caprese Sandwich", .type = FoodType::eSandwich, .cost = 2.50 }},
        {19, {.foodID = 19, .name = "Veggie Sandwich with Hummus", .type = FoodType::eSandwich, .cost = 2.50 }},
        {20, {.foodID = 20, .name = "Water", .type = FoodType::eBeverage, .cost = 0.00 }},
        {21, {.foodID = 21, .name = "Hot Chocolate", .type = FoodType::eBeverage, .cost = 1.50 }},
        {22, {.foodID = 22, .name = "Green Tea", .type = FoodType::eBeverage, .cost = 1.00 }},
        {23, {.foodID = 23, .name = "Vanilla Chai Black Tea", .type = FoodType::eBeverage, .cost = 1.00 }},
        {24, {.foodID = 24, .name = "Peppermint Herbal Tea", .type = FoodType::eBeverage, .cost = 1.00 }},
        {25, {.foodID = 25, .name = "White Chocolate Macadamia Nut Cookie", .type = FoodType::eCookie, .cost = 1.00 }},
        {26, {.foodID = 26, .name = "Chocolate Chip Cookie", .type = FoodType::eCookie, .cost = 1.00 }},
    };

    return foods;
}

//#define PerfGenerate
#ifdef PerfGenerate
Hashtable<Transaction> GenerateTransactions(std::size_t amount)
{
#ifdef SeededRandom
    Random random{ 777 };
#else
    Random random;
#endif

    Hashtable<Transaction> transactions;
    if (amount <= 0)
        return transactions;

    const auto DoWork = [&transactions](int minID, int maxID, Random& random)
    {
        static std::mutex workMutex;
        while (minID <= maxID)
        {
            std::scoped_lock<std::mutex> lock{ workMutex };

            transactions.emplace(minID, Transaction{
                .orderNumber = minID,
                .gratuity = GenerateGratuity(random),
                .purchases = GenerateTicket(GenerateFoods(), random)
            });

            minID++;
        }
    };

    const int numThreads = std::jthread::hardware_concurrency();
    const int extras = static_cast<int>(amount % numThreads);
    const int blockSize = static_cast<int>((amount - extras) / numThreads);
    
    //std::vector<std::future<void>> futures;
    //for (auto threadIndex = 0; threadIndex < numThreads; ++threadIndex)
    //{
    //    const int extraAmount = (threadIndex == numThreads - 1 ? extras : 0);
    //    const int minRange = blockSize * threadIndex;
    //    const int maxRange = (threadIndex + 1) * blockSize - 1 + extraAmount;
    //
    //    std::async(std::launch::async, DoWork, minRange, maxRange, std::ref(random));
    //}
   
    std::future<void> future1 = std::async(std::launch::async, DoWork, blockSize * 1, (1 + 1) * blockSize - 1, std::ref(random));
    std::future<void> future2 = std::async(std::launch::async, DoWork, blockSize * 2, (2 + 1) * blockSize - 1, std::ref(random));
    std::future<void> future3 = std::async(std::launch::async, DoWork, blockSize * 3, (3 + 1) * blockSize - 1, std::ref(random));
    std::future<void> future4 = std::async(std::launch::async, DoWork, blockSize * 4, (4 + 1) * blockSize - 1, std::ref(random));

    //std::ranges::for_each(futures, [](const std::future<void>& future) { future.wait(); });

    return transactions;
}
#else
Hashtable<Transaction> GenerateTransactions(std::size_t amount)
{
#ifdef SeededRandom
    Random random{ 777 };
#else
    Random random;
#endif

    Hashtable<Transaction> transactions;
    if (amount <= 0)
        return transactions;

    for (auto index = 0; index < amount; ++index)
    {
        transactions.emplace(index, Transaction{
            .orderNumber = index,
            .gratuity = GenerateGratuity(random),
            .purchases = GenerateTicket(GenerateFoods(), random)
        });
    }

    return transactions;
}
#endif


MultiHashtable<PurchaseMapping> GeneratePurchaseMapping(const Hashtable<Transaction>& transactions)
{
    MultiHashtable<PurchaseMapping> purchaseMapping;
    if (transactions.empty())
        return purchaseMapping;

    const bakery::Hashtable<FoodItem>& foods = GenerateFoods();
    for (const auto& [orderNumber, transaction] : transactions)
    {
        for (int foodID : transaction.GetPurchases())
        {
            if (foods.contains(foodID))
                purchaseMapping.emplace(transaction.orderNumber, PurchaseMapping{ foodID, transaction.orderNumber });
        }
    }

    return purchaseMapping;
}

std::vector<int> Transaction::GetPurchases() const
{
    std::vector<int> ret;
    if (purchases.count() > 4)
        throw std::logic_error("There were more than 4 purchases on a transaction.");

    for (const auto& [foodID, _] : GenerateFoods())
    {
        if (purchases.test(foodID))
            ret.push_back(foodID);
    }

    return ret;
}

Database::Database()
    : m_foods(GenerateFoods())
{}

Database::Database(std::size_t amount)
    : m_foods(GenerateFoods()), m_transactions(GenerateTransactions(amount))
{}

void Database::Save(const std::filesystem::path& directory) const
{
    if (!std::filesystem::is_directory(directory))
        return;

    const std::filesystem::path transDBPath = directory / "transactions.csv";
    const std::filesystem::path purchasedDBPath = directory / "purchaseMappings.csv";

    std::ofstream transactionsDB{ transDBPath };
    std::ofstream purchasedItemsDB{ purchasedDBPath };

    for (const auto& [_, transaction] : m_transactions)
        transactionsDB << transaction;

    for (const auto& [_, purchaseMapping] : GeneratePurchaseMapping(m_transactions))
        purchasedItemsDB << purchaseMapping;
}

bool Database::Load(const std::filesystem::path& directory)
{
    if (!std::filesystem::is_directory(directory))
        return false;

    const std::filesystem::path transDBPath = directory / "transactions.csv";
    const std::filesystem::path purchasedDBPath = directory / "purchaseMappings.csv";

    if (!std::filesystem::exists(transDBPath) ||
        !std::filesystem::exists(purchasedDBPath))
    {
        return false;
    }

    MultiHashtable<PurchaseMapping> purchaseMapping;

    std::ifstream transactionsDB{ transDBPath };
    std::ifstream purchasedItemsDB{ purchasedDBPath };

    std::for_each(std::istream_iterator<Transaction>(transactionsDB), std::istream_iterator<Transaction>(),
        [this](const Transaction& item) { m_transactions[item.orderNumber] = item; });

    std::for_each(std::istream_iterator<PurchaseMapping>(purchasedItemsDB), std::istream_iterator<PurchaseMapping>(),
        [&purchaseMapping](const PurchaseMapping& item) { purchaseMapping.emplace(item.orderNumber, item); });

    if (purchaseMapping.empty())
        return false;

    for (auto& [orderNumber, transaction] : m_transactions)
    {
        const auto& [begin, end] = purchaseMapping.equal_range(orderNumber);
        std::for_each(begin, end, [this, &transaction](const std::pair<int, PurchaseMapping>& pair) {
            transaction.purchases.set(pair.second.foodID);
        });
    }

    return true;
}

bool Database::CleanDisk(const std::filesystem::path& directory) const
{
    if (!std::filesystem::is_directory(directory))
        return false;

    const std::filesystem::path foodDBPath = directory / "foods.csv";
    const std::filesystem::path transDBPath = directory / "transactions.csv";
    const std::filesystem::path purchasedDBPath = directory / "purchaseMappings.csv";

    return std::filesystem::remove(foodDBPath) &&
           std::filesystem::remove(transDBPath) &&
           std::filesystem::remove(purchasedDBPath);
}
}
