#pragma once
#include "Order.h"
#include "MemoryPool.h" // NEW
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    std::vector<Trade> addOrder(Order order);
    void cancelOrder(OrderID orderId);

    // Add these under your public functions in OrderBook.h
    std::optional<Price> getBestBid() const {
        if (bids.empty()) return std::nullopt;
        return bids.begin()->first; // Highest price is at the top of the descending map
    }

    std::optional<Price> getBestAsk() const {
        if (asks.empty()) return std::nullopt;
        return asks.begin()->first; // Lowest price is at the top of the ascending map
    }

private:
    // The Engine's Private Vending Machine
    MemoryPool memoryPool;

    // Notice we now map Price -> PriceLevel (our custom struct)
    std::map<Price, PriceLevel, std::greater<Price>> bids;
    std::map<Price, PriceLevel> asks;

    // Fast Lookup: OrderID -> Index in the Memory Pool
    std::unordered_map<OrderID, int32_t> orderMap;
};