#pragma once
#include <cstdint>

enum class Side {
    Buy,
    Sell
};

using OrderID = uint64_t;
using Price = uint64_t;    
using Quantity = uint32_t;

struct Trade {
    Quantity quantity;
    Price price;
    OrderID buyerId;
    OrderID sellerId;
};

struct PriceLevel {
    int32_t headIndex = -1; 
    int32_t tailIndex = -1;
};
