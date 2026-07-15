#pragma once
#include "Order.h"
#include "MemoryPool.h"
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

    std::optional<Price> getBestBid() const {
        if (bids.empty()) return std::nullopt;
        return bids.begin()->first; 
    }

    std::optional<Price> getBestAsk() const {
        if (asks.empty()) return std::nullopt;
        return asks.begin()->first; 
    }

private:
    MemoryPool memoryPool;

    std::map<Price, PriceLevel, std::greater<Price>> bids;
    std::map<Price, PriceLevel> asks;

    std::unordered_map<OrderID, int32_t> orderMap;
};
