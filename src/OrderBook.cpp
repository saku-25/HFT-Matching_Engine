#include "../include/OrderBook.h"
#include <iostream>
#include <algorithm> // Needed for std::min


std::vector<Trade> OrderBook::addOrder(Order order) {
    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        
        // --- 1. MATCHING LOOP ---
        while (order.quantity > 0 && !asks.empty()) {
            auto bestAskIter = asks.begin();
            Price bestAskPrice = bestAskIter->first;
            PriceLevel& level = bestAskIter->second;

            if (order.price < bestAskPrice) break;

            int32_t currentAskIndex = level.headIndex;
            
            // Walk our custom linked list using indices
            while (currentAskIndex != NULL_INDEX && order.quantity > 0) {
                Order& restingAsk = memoryPool.get(currentAskIndex);
                Quantity tradeQty = std::min(order.quantity, restingAsk.quantity);

                trades.push_back({tradeQty, bestAskPrice, order.id, restingAsk.id});

                order.quantity -= tradeQty;
                restingAsk.quantity -= tradeQty;

                if (restingAsk.quantity == 0) {
                    // Save next index before we destroy this node
                    int32_t nextIndex = restingAsk.nextOrderIndex; 
                    orderMap.erase(restingAsk.id);

                    // --- THE UNLINKING (Closing the gap) ---
                    if (restingAsk.prevOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingAsk.prevOrderIndex).nextOrderIndex = restingAsk.nextOrderIndex;
                    } else {
                        level.headIndex = restingAsk.nextOrderIndex; // It was the head
                    }
                    
                    if (restingAsk.nextOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingAsk.nextOrderIndex).prevOrderIndex = restingAsk.prevOrderIndex;
                    } else {
                        level.tailIndex = restingAsk.prevOrderIndex; // It was the tail
                    }

                    // Recycle the memory instantly!
                    memoryPool.deallocate(currentAskIndex);
                    currentAskIndex = nextIndex;
                } else {
                    currentAskIndex = restingAsk.nextOrderIndex;
                }
            }

            // Clean up empty price levels
            if (level.headIndex == NULL_INDEX) {
                asks.erase(bestAskIter);
            }
        }

        // --- 2. RESTING LOOP (Adding to the Pool) ---
        if (order.quantity > 0) {
            int32_t newOrderIndex = memoryPool.allocate();
            Order& newOrder = memoryPool.get(newOrderIndex);
            
            newOrder = order; // Copy data into the pool
            newOrder.nextOrderIndex = NULL_INDEX;
            newOrder.prevOrderIndex = NULL_INDEX;

            PriceLevel& level = bids[order.price];
            
            if (level.headIndex == NULL_INDEX) {
                // First order at this price
                level.headIndex = newOrderIndex;
                level.tailIndex = newOrderIndex;
            } else {
                // Append to the back of the line (Time Priority)
                newOrder.prevOrderIndex = level.tailIndex;
                memoryPool.get(level.tailIndex).nextOrderIndex = newOrderIndex;
                level.tailIndex = newOrderIndex;
            }
            orderMap[order.id] = newOrderIndex;
        }

    } else {
        // --- SIDE::SELL (The Exact Mirror Image) ---
        while (order.quantity > 0 && !bids.empty()) {
            auto bestBidIter = bids.begin();
            Price bestBidPrice = bestBidIter->first;
            PriceLevel& level = bestBidIter->second;

            if (order.price > bestBidPrice) break;

            int32_t currentBidIndex = level.headIndex;
            while (currentBidIndex != NULL_INDEX && order.quantity > 0) {
                Order& restingBid = memoryPool.get(currentBidIndex);
                Quantity tradeQty = std::min(order.quantity, restingBid.quantity);

                trades.push_back({tradeQty, bestBidPrice, restingBid.id, order.id});
                order.quantity -= tradeQty;
                restingBid.quantity -= tradeQty;

                if (restingBid.quantity == 0) {
                    int32_t nextIndex = restingBid.nextOrderIndex;
                    orderMap.erase(restingBid.id);

                    if (restingBid.prevOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingBid.prevOrderIndex).nextOrderIndex = restingBid.nextOrderIndex;
                    } else {
                        level.headIndex = restingBid.nextOrderIndex;
                    }
                    if (restingBid.nextOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingBid.nextOrderIndex).prevOrderIndex = restingBid.prevOrderIndex;
                    } else {
                        level.tailIndex = restingBid.prevOrderIndex;
                    }

                    memoryPool.deallocate(currentBidIndex);
                    currentBidIndex = nextIndex;
                } else {
                    currentBidIndex = restingBid.nextOrderIndex;
                }
            }

            if (level.headIndex == NULL_INDEX) bids.erase(bestBidIter);
        }

        if (order.quantity > 0) {
            int32_t newOrderIndex = memoryPool.allocate();
            Order& newOrder = memoryPool.get(newOrderIndex);
            newOrder = order;
            newOrder.nextOrderIndex = NULL_INDEX;
            newOrder.prevOrderIndex = NULL_INDEX;

            PriceLevel& level = asks[order.price];
            if (level.headIndex == NULL_INDEX) {
                level.headIndex = newOrderIndex;
                level.tailIndex = newOrderIndex;
            } else {
                newOrder.prevOrderIndex = level.tailIndex;
                memoryPool.get(level.tailIndex).nextOrderIndex = newOrderIndex;
                level.tailIndex = newOrderIndex;
            }
            orderMap[order.id] = newOrderIndex;
        }
    }

    return trades;
}


void OrderBook::cancelOrder(OrderID orderId) {
    auto mapIterator = orderMap.find(orderId);
    if (mapIterator == orderMap.end()) return; // Order doesn't exist

    // 1. Get the order index from our hash map
    int32_t orderIndex = mapIterator->second;
    Order& orderToCancel = memoryPool.get(orderIndex);

    // 2. Find which price level this order belongs to
    PriceLevel* level = nullptr;
    if (orderToCancel.side == Side::Buy) {
        level = &bids[orderToCancel.price];
    } else {
        level = &asks[orderToCancel.price];
    }

    // 3. The Unlinking (O(1) deletion)
    if (orderToCancel.prevOrderIndex != NULL_INDEX) {
        memoryPool.get(orderToCancel.prevOrderIndex).nextOrderIndex = orderToCancel.nextOrderIndex;
    } else {
        // If there was no previous order, this was the Head
        level->headIndex = orderToCancel.nextOrderIndex;
    }

    if (orderToCancel.nextOrderIndex != NULL_INDEX) {
        memoryPool.get(orderToCancel.nextOrderIndex).prevOrderIndex = orderToCancel.prevOrderIndex;
    } else {
        // If there was no next order, this was the Tail
        level->tailIndex = orderToCancel.prevOrderIndex;
    }

    // 4. Memory Cleanup: Delete the price level if it is completely empty
    if (level->headIndex == NULL_INDEX) {
        if (orderToCancel.side == Side::Buy) {
            bids.erase(orderToCancel.price);
        } else {
            asks.erase(orderToCancel.price);
        }
    }

    // 5. Remove from fast-lookup map and recycle the memory ticket
    orderMap.erase(mapIterator);
    memoryPool.deallocate(orderIndex);
}