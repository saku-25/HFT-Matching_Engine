#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <iomanip>
#include "../include/OrderBook.h"
#include "../include/ThreadSafeQueue.h"
#include "../include/sqlite3.h"

// ========================================================================
// THE LOGGER THREAD (Database Engineering)
// ========================================================================
void loggerThreadFunction(ThreadSafeQueue<Trade>& tradeQueue) {
    std::cout << "[LOGGER] Booting up Optimized Database Engine...\n";

    sqlite3* db;
    sqlite3_open("ledger.db", &db);

    // 1. Enable Write-Ahead Logging for maximum disk performance
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", NULL, 0, NULL);
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, 0, NULL);

    // 2. Create the Schema
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS Trades (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            buyer_id INTEGER NOT NULL,
            seller_id INTEGER NOT NULL,
            price INTEGER NOT NULL,
            quantity INTEGER NOT NULL,
            execution_time DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    sqlite3_exec(db, createTableSQL, NULL, 0, NULL);

    // 3. The Prepared Statement (Zero string allocation)
    const char* insertSQL = "INSERT INTO Trades (buyer_id, seller_id, price, quantity) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, insertSQL, -1, &stmt, NULL);

    // 4. Start a mass transaction block
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, 0, NULL);
    
    int batchCount = 0;

    // 5. The High-Speed Logging Loop
    while (true) {
        std::optional<Trade> tradeOpt = tradeQueue.pop();
        
        if (!tradeOpt.has_value()) {
            std::cout << "[LOGGER] Shutting down Database Connection...\n";
            break; 
        }

        Trade trade = tradeOpt.value();
        
        // Inject integers directly into the compiled SQL statement
        sqlite3_bind_int(stmt, 1, trade.buyerId);
        sqlite3_bind_int(stmt, 2, trade.sellerId);
        sqlite3_bind_int(stmt, 3, trade.price);
        sqlite3_bind_int(stmt, 4, trade.quantity);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);

        // Batching: Flush to physical hard drive every 10,000 trades
        batchCount++;
        if (batchCount >= 10000) {
            sqlite3_exec(db, "COMMIT;", NULL, 0, NULL);
            sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, 0, NULL);
            batchCount = 0;
            std::cout << "[LOGGER] Flushed 10,000 trades to disk.\n";
        }
    }

    // 6. Clean up
    sqlite3_exec(db, "COMMIT;", NULL, 0, NULL);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// ========================================================================
// THE ENGINE & MARKET SIMULATION (Main Thread)
// ========================================================================
int main() {
    OrderBook engine;
    ThreadSafeQueue<Trade> tradeQueue;
    
    // Spin up the background database thread
    std::thread loggerThread(loggerThreadFunction, std::ref(tradeQueue));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "\n[QUANT] Initializing Market Simulation with Market Maker...\n";

    // Random Number Generators (Monte Carlo Setup)
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::normal_distribution<> priceDist(15000.0, 50.0); // Bell curve around $150.00
    std::uniform_int_distribution<> qtyDist(10, 100);    // 10 to 100 shares
    std::uniform_int_distribution<> sideDist(0, 1);      // 50/50 Buy or Sell

    // Market Maker Tracking Variables
    int orderIdCounter = 1;
    int marketMakerProfit = 0;
    int marketMakerInventory = 0; 
    int totalSharesBought = 0;
    int totalSharesSold = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Run 100 loops (Trading Epochs)
    for (int epoch = 0; epoch < 100; ++epoch) {
        
        // --- 1. THE INTELLIGENT MARKET MAKER WAKES UP ---
        auto bestBidOpt = engine.getBestBid();
        auto bestAskOpt = engine.getBestAsk();

        if (bestBidOpt.has_value() && bestAskOpt.has_value()) {
            Price bid = bestBidOpt.value();
            Price ask = bestAskOpt.value();
            
            double midPrice = (bid + ask) / 2.0;

            // The Avellaneda-Stoikov Inventory Skew
            double riskAversion = 0.05; 
            double skew = marketMakerInventory * riskAversion;
            double reservationPrice = midPrice - skew;

            // Quote around our risk-adjusted reservation price
            Price myBid = static_cast<Price>(reservationPrice - 1);
            Price myAsk = static_cast<Price>(reservationPrice + 1);

            // Prevent crossed markets
            if (myBid < myAsk) {
                auto mmBuyTrades = engine.addOrder({static_cast<OrderID>(orderIdCounter++), 0, myBid, 50, 42, Side::Buy});
                for (const auto& t : mmBuyTrades) tradeQueue.push(t);

                auto mmSellTrades = engine.addOrder({static_cast<OrderID>(orderIdCounter++), 0, myAsk, 50, 42, Side::Sell});
                for (const auto& t : mmSellTrades) tradeQueue.push(t);
            }
        }

        // --- 2. THE NOISE TRADERS ARRIVE ---
        // Fire 100 random retail orders into the engine
        for (int i = 0; i < 100; ++i) {
            Price randomPrice = static_cast<Price>(priceDist(gen));
            Quantity randomQty = qtyDist(gen);
            Side randomSide = (sideDist(gen) == 0) ? Side::Buy : Side::Sell;

            std::vector<Trade> executions = engine.addOrder({
                static_cast<OrderID>(orderIdCounter++), 
                0, 
                randomPrice, 
                randomQty, 
                99, // Trader ID 99 (Retail)
                randomSide
            });

            for (const auto& trade : executions) {
                tradeQueue.push(trade);
                
                // Track Market Maker PnL and Volume if they were involved
                if (trade.buyerId == 42) {
                    marketMakerInventory += trade.quantity;
                    marketMakerProfit -= (trade.price * trade.quantity); 
                    totalSharesBought += trade.quantity;                 
                } 
                else if (trade.sellerId == 42) {
                    marketMakerInventory -= trade.quantity;
                    marketMakerProfit += (trade.price * trade.quantity); 
                    totalSharesSold += trade.quantity;                   
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "[ENGINE] Matched 10,000 orders in " << duration.count() << " milliseconds!\n";

    // --- 3. FINAL REPORT & MARK-TO-MARKET PnL ---
    double finalMidPrice = 15000.0; // Fallback to $150.00
    auto finalBid = engine.getBestBid();
    auto finalAsk = engine.getBestAsk();
    if (finalBid.has_value() && finalAsk.has_value()) {
        finalMidPrice = (finalBid.value() + finalAsk.value()) / 2.0;
    }

    double trueProfitCents = marketMakerProfit + (marketMakerInventory * finalMidPrice);
    double trueProfitDollars = trueProfitCents / 100.0;
    double rawCashDollars = marketMakerProfit / 100.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n========================================\n";
    std::cout << "      MARKET MAKER FINAL REPORT\n";
    std::cout << "========================================\n";
    std::cout << "Total Shares Bought : " << totalSharesBought << "\n";
    std::cout << "Total Shares Sold   : " << totalSharesSold << "\n";
    std::cout << "Final Inventory     : " << marketMakerInventory << " shares ";
    
    if (marketMakerInventory > 0) std::cout << "(LONG)\n";
    else if (marketMakerInventory < 0) std::cout << "(SHORT)\n";
    else std::cout << "(FLAT)\n";

    std::cout << "----------------------------------------\n";
    std::cout << "Raw Cash Balance    : $" << rawCashDollars << "\n";
    std::cout << "Mark-to-Market PnL  : $" << trueProfitDollars << "\n";
    std::cout << "========================================\n\n";

    // 4. Graceful Shutdown
    tradeQueue.shutdown();
    loggerThread.join();

    return 0;
}