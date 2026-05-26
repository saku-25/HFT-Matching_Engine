#pragma once
#include "Order.h"
#include <vector>
#include <iostream>

class MemoryPool {
private:
    std::vector<Order> pool;           // The massive contiguous block of RAM
    std::vector<int32_t> freeIndices;  // A stack of available slot numbers

public:
    // When the exchange boots, we request all the memory we will ever need
    explicit MemoryPool(size_t capacity = 1000000) {
        pool.resize(capacity);
        freeIndices.reserve(capacity);
        
        // Populate the free list. 
        // We push backwards (capacity down to 0) so that when we pop(), 
        // we get index 0, then 1, then 2. This perfectly aligns with CPU Cache!
        for (int32_t i = static_cast<int32_t>(capacity) - 1; i >= 0; --i) {
            freeIndices.push_back(i);
        }
    }

    // O(1) Allocation - No OS System Calls!
    int32_t allocate() {
        if (freeIndices.empty()) {
            std::cerr << "[CRITICAL] Engine Memory Pool Exhausted!\n";
            return NULL_INDEX; 
        }
        
        int32_t index = freeIndices.back();
        freeIndices.pop_back();
        return index;
    }

    // O(1) Deallocation - Instant recycling
    void deallocate(int32_t index) {
        // Scrub the intrusive pointers so old data doesn't corrupt new orders
        pool[index].nextOrderIndex = NULL_INDEX;
        pool[index].prevOrderIndex = NULL_INDEX;
        
        // Put the ticket back in the stack for the next order
        freeIndices.push_back(index);
    }

    // Fast memory access by index
    Order& get(int32_t index) {
        return pool[index];
    }
};