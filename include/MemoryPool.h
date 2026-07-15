#pragma once
#include "Order.h"
#include <vector>
#include <iostream>

class MemoryPool {
private:
    std::vector<Order> pool;
    std::vector<int32_t> freeIndices;

public:
    explicit MemoryPool(size_t capacity = 1000000) {
        pool.resize(capacity);
        freeIndices.reserve(capacity);
        
        for (int32_t i = static_cast<int32_t>(capacity) - 1; i >= 0; --i) {
            freeIndices.push_back(i);
        }
    }

    int32_t allocate() {
        if (freeIndices.empty()) {
            std::cerr << "[CRITICAL] Engine Memory Pool Exhausted!\n";
            return NULL_INDEX; 
        }
        
        int32_t index = freeIndices.back();
        freeIndices.pop_back();
        return index;
    }

    void deallocate(int32_t index) {
        pool[index].nextOrderIndex = NULL_INDEX;
        pool[index].prevOrderIndex = NULL_INDEX;
        
        freeIndices.push_back(index);
    }

    Order& get(int32_t index) {
        return pool[index];
    }
};
