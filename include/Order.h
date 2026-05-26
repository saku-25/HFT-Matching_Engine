#pragma once
#include "Types.h"

// We use -1 to represent a "null" index (since 0 is a valid array index)
constexpr int32_t NULL_INDEX = -1; 

struct Order {
    OrderID id;           // 8 bytes
    uint64_t timestamp;   // 8 bytes
    Price price;          // 8 bytes
    Quantity quantity;    // 4 bytes
    uint32_t traderId;    // 4 bytes
    Side side;            // 1 byte (enum)
    
    // --- NEW: Intrusive Linked List Indices ---
    // These replace the need for std::list
    int32_t nextOrderIndex = NULL_INDEX; 
    int32_t prevOrderIndex = NULL_INDEX; 
};