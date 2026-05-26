#pragma once
#include <cstdint>

// Using enum class for strict type safety
enum class Side {
    Buy,
    Sell
};

// Type aliases for readability and easy modification later
using OrderID = uint64_t;
using Price = uint64_t;    // Prices are often stored as integers (e.g., $1.50 -> 150) to avoid floating-point errors
using Quantity = uint32_t;

// Add this below your existing Type definitions in Types.h

struct Trade {
    Quantity quantity;
    Price price;
    OrderID buyerId;
    OrderID sellerId;
};

// This replaces std::list!
// It simply holds the start and end indices of the intrusive linked list.
struct PriceLevel {
    int32_t headIndex = -1; // -1 means NULL_INDEX
    int32_t tailIndex = -1;
};