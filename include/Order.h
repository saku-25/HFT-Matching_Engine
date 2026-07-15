#pragma once
#include "Types.h"

constexpr int32_t NULL_INDEX = -1; 
/*
This file just describe the structure of Order Variable
*/    
struct Order {
    OrderID id;           // 8 bytes
    uint64_t timestamp;   // 8 bytes
    Price price;          // 8 bytes
    Quantity quantity;    // 4 bytes
    uint32_t traderId;    // 4 bytes
    Side side;            // 1 byte (enum)
    
    int32_t nextOrderIndex = NULL_INDEX; 
    int32_t prevOrderIndex = NULL_INDEX; 
};
