#include "bakery.h"
#include "queries.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <functional>
#include <queue>

#include <future>

int main()
{
    const auto result = bakery::GenerateTransactionsParallel(100);

    int i = 0;
   // bakery::Database database{1'000'000};
    
    //queries::MapReduceParallel query{ database, 1000 };
    //auto result = query.GetGreatestAndLeastPopularItems();
}