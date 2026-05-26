# High-Frequency Trading Matching Engine

A bare-metal, low-latency limit order book (LOB) and market-making simulation built entirely in modern C++17. 

This project simulates a high-throughput financial exchange, separating the latency-sensitive matching engine from the I/O-bound database logger using lock-based multithreading, and features a quantitative market-making bot that manages inventory risk using the Avellaneda-Stoikov model.

## 🚀 Core Architecture & Features

### 1. Zero-Allocation Matching Engine
* **O(1) Order Matching:** Engineered a custom memory pool and intrusive doubly-linked lists to entirely eliminate OS heap allocations (`new`/`delete`) on the critical path.
* **Cache Locality:** Contiguous memory arrays prevent CPU cache misses, drastically reducing latency compared to standard library containers like `std::list`.
* **Price-Time Priority:** Standard FIFO execution logic for resting limit orders.

### 2. Concurrent Systems Design
* **Producer-Consumer Model:** Split the architecture into two distinct threads running on separate CPU cores to ensure database I/O never blocks the matching engine.
* **Thread-Safe Queue:** Implemented using `std::mutex` and `std::condition_variable` to safely pass trade execution reports across threads.

### 3. Persistent Ledger (Database Engineering)
* **Embedded SQLite:** Serverless relational database compiled directly into the executable.
* **High-Throughput Batching:** Utilizes Write-Ahead Logging (WAL) and compiled Prepared Statements to flush tens of thousands of trades to disk in a single transaction block.

### 4. Quantitative Market Simulation
* **Monte Carlo Noise Traders:** Uses `<random>` standard normal distributions (Gaussian) to simulate realistic retail market flow around a defined fair value.
* **Avellaneda-Stoikov Market Maker:** An intelligent liquidity-providing bot that calculates a reservation price and dynamically skews its bid/ask quotes to manage inventory risk and prevent bankruptcy during directional market crashes.

## 🛠️ Build Instructions (Windows / MinGW POSIX)

This engine requires a compiler that supports C++17 and the POSIX threading model. 

```bash
# Clone the repository
git clone [https://github.com/YOUR_USERNAME/hft-matching-engine.git](https://github.com/YOUR_USERNAME/hft-matching-engine.git)
cd hft-matching-engine

# Create build directory
mkdir build
cd build

# Generate Makefiles and build
cmake -G "MinGW Makefiles" ..
mingw32-make

# Run the simulation
.\matching_engine.exe
