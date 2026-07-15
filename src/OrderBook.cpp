#include "../include/OrderBook.h"
#include <iostream>
#include <algorithm> 

std::vector<Trade> OrderBook::addOrder(Order &order) {
    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        
        while (order.quantity > 0 && !asks.empty()) {
            auto bestAskIter = asks.begin();
            Price bestAskPrice = bestAskIter->first;
            PriceLevel& level = bestAskIter->second;

            if (order.price < bestAskPrice) break;

            int32_t currentAskIndex = level.headIndex;
            
            while (currentAskIndex != NULL_INDEX && order.quantity > 0) {
                Order& restingAsk = memoryPool.get(currentAskIndex);
                Quantity tradeQty = std::min(order.quantity, restingAsk.quantity);

                trades.push_back({tradeQty, bestAskPrice, order.id, restingAsk.id});

                order.quantity -= tradeQty;
                restingAsk.quantity -= tradeQty;

                if (restingAsk.quantity == 0) {
                    int32_t nextIndex = restingAsk.nextOrderIndex; 
                    orderMap.erase(restingAsk.id);

                    if (restingAsk.prevOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingAsk.prevOrderIndex).nextOrderIndex = restingAsk.nextOrderIndex;
                    } else {
                        level.headIndex = restingAsk.nextOrderIndex; 
                    }
                    
                    if (restingAsk.nextOrderIndex != NULL_INDEX) {
                        memoryPool.get(restingAsk.nextOrderIndex).prevOrderIndex = restingAsk.prevOrderIndex;
                    } else {
                        level.tailIndex = restingAsk.prevOrderIndex; 
                    }

                    memoryPool.deallocate(currentAskIndex);
                    currentAskIndex = nextIndex;
                } else {
                    currentAskIndex = restingAsk.nextOrderIndex;
                }
            }

            if (level.headIndex == NULL_INDEX) {
                asks.erase(bestAskIter);
            }
        }

        if (order.quantity > 0) {
            int32_t newOrderIndex = memoryPool.allocate();
            Order& newOrder = memoryPool.get(newOrderIndex);
            
            newOrder = order; 
            newOrder.nextOrderIndex = NULL_INDEX;
            newOrder.prevOrderIndex = NULL_INDEX;

            PriceLevel& level = bids[order.price];
            
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

    } else {
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
    if (mapIterator == orderMap.end()) return; 

    int32_t orderIndex = mapIterator->second;
    Order& orderToCancel = memoryPool.get(orderIndex);

    PriceLevel* level = nullptr;
    if (orderToCancel.side == Side::Buy) {
        level = &bids[orderToCancel.price];
    } else {
        level = &asks[orderToCancel.price];
    }

    if (orderToCancel.prevOrderIndex != NULL_INDEX) {
        memoryPool.get(orderToCancel.prevOrderIndex).nextOrderIndex = orderToCancel.nextOrderIndex;
    } else {
        level->headIndex = orderToCancel.nextOrderIndex;
    }

    if (orderToCancel.nextOrderIndex != NULL_INDEX) {
        memoryPool.get(orderToCancel.nextOrderIndex).prevOrderIndex = orderToCancel.prevOrderIndex;
    } else {
        level->tailIndex = orderToCancel.prevOrderIndex;
    }

    if (level->headIndex == NULL_INDEX) {
        if (orderToCancel.side == Side::Buy) {
            bids.erase(orderToCancel.price);
        } else {
            asks.erase(orderToCancel.price);
        }
    }

    orderMap.erase(mapIterator);
    memoryPool.deallocate(orderIndex);
}
