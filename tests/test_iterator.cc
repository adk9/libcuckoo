#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <utility>

#include <libcuckoo/cuckoohash_map.hh>
#include "test_util.cc"

typedef uint32_t KeyType;
typedef uint32_t ValType;
typedef cuckoohash_map<KeyType, ValType> Table;

const size_t power = 4;
const size_t size = 1L << power;

class IteratorEnvironment {
public:
    IteratorEnvironment()
        : emptytable(size), table(size), items_end(items+size) {
        // Fills up table and items with random values
        uint64_t seed =
            std::chrono::system_clock::now().time_since_epoch().count();
        std::cout << "seed = " << seed << std::endl;
        std::uniform_int_distribution<ValType> v_dist(
            std::numeric_limits<ValType>::min(),
            std::numeric_limits<ValType>::max());
        std::mt19937_64 gen(seed);
        for (size_t i = 0; i < size; i++) {
            items[i] = v_dist(gen);
            EXPECT_TRUE(table.insert(i, items[i]));
        }
    }

    Table emptytable;
    Table table;
    ValType items[size];
    ValType* items_end;
};

IteratorEnvironment* iter_env;

void EmptyTableBeginEndIterator() {
    Table emptytable(size);
    Table::locked_table lt = emptytable.lock_table();
    Table::locked_table::const_iterator it = lt.cbegin();
    ASSERT_TRUE(it == lt.cend());
    lt.release();
    lt = emptytable.lock_table();
    it = lt.cend();
    ASSERT_TRUE(it == lt.cbegin());
}

void FilledTableIterForwards() {
    bool visited[size] = {};
    auto lt = iter_env->table.lock_table();
    for (const auto& item : lt) {
        auto itemiter = std::find(iter_env->items, iter_env->items_end,
                                  item.second);
        EXPECT_NE(itemiter, iter_env->items_end);
        visited[iter_env->items_end-itemiter-1] = true;
    }
    // Checks that all the items were visited
    for (size_t i = 0; i < size; i++) {
        EXPECT_TRUE(visited[i]);
    }
}

void FilledTableIterBackwards() {
    auto lt = iter_env->table.lock_table();
    auto it = lt.cend();
    bool visited[size] = {};
    do {
        it--;
        auto beg = lt.cbegin();
        auto itemiter = std::find(iter_env->items, iter_env->items_end,
                                  it->second);
        EXPECT_NE(itemiter, iter_env->items_end);
        visited[iter_env->items_end-itemiter-1] = true;
    } while (it != lt.cbegin());
    // Checks that all the items were visited
    for (size_t i = 0; i < size; i++) {
        EXPECT_TRUE(visited[i]);
    }
}

void FilledTableIncrementItems() {
    for (size_t i = 0; i < size; i++) {
        iter_env->items[i]++;
    }
    // Also tests casting from a const iterator to a mutable one
    auto lt = iter_env->table.lock_table();
    for (auto t_mut = lt.begin(); t_mut != lt.end(); ++t_mut) {
        t_mut->second++;
        EXPECT_NE(std::find(iter_env->items, iter_env->items_end,
                            t_mut->second),
                  iter_env->items_end);
    }
}

int main() {
    iter_env = new IteratorEnvironment;
    std::cout << "Running EmptyTableBeginEndIterator" << std::endl;
    EmptyTableBeginEndIterator();
    std::cout << "Running FilledTableIterBackwards" << std::endl;
    FilledTableIterBackwards();
    std::cout << "Running FilledTableIterForwards" << std::endl;
    FilledTableIterForwards();
    std::cout << "Running FilledTableIncrementItems" << std::endl;
    FilledTableIncrementItems();
    return main_return_value;
}
