/**
 * @file      MemoryBenchmark.cpp
 * @author	  Jethro Sung
 * @email	  sung.h
 * @date	  04/04/26
 *
 * @brief     Benchmarks the custom FixedBlockPool allocator against standard
 *            dynamic allocation patterns.
 *
 *            This file compares raw new/delete and unordered_map-based
 *            allocation against the engine memory pool in order to measure
 *            allocation and destruction performance for component-like objects.
 */
#include "Benchmarks/MemoryBenchmark.h"
#include "Core/Memory/FixedBlockPool.h"
#include "Input/DebugConsole.hpp"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <string>

// Dummy component for testing (64 bytes)
struct BenchmarkComponent {
    float data[16];
    BenchmarkComponent() {
        for(int i=0; i<16; ++i) data[i] = 0.0f;
    }
};

using namespace eng::mem;

namespace Benchmarks {

    void RunMemoryBenchmark() {
        const int ITERATIONS = 100000;
        
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "\n=== Memory Benchmark (Iterations: ", ITERATIONS, ") ===\n");

        // new/delete
        {
            auto start = std::chrono::high_resolution_clock::now();
            std::vector<BenchmarkComponent*> ptrs;
            ptrs.reserve(ITERATIONS);
            
            for (int i = 0; i < ITERATIONS; ++i) {
                ptrs.push_back(new BenchmarkComponent());
            }
            
            for (auto p : ptrs) {
                delete p;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = end - start;
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Raw new/delete: ", diff.count(), " ms\n");
        }

        // FixedBlockPool
        {
            FixedBlockPool<BenchmarkComponent, 1024> pool;
            pool.ReservePages(ITERATIONS / 1024 + 1);

            auto start = std::chrono::high_resolution_clock::now();
            std::vector<BenchmarkComponent*> ptrs;
            ptrs.reserve(ITERATIONS);
            
            for (int i = 0; i < ITERATIONS; ++i) {
                ptrs.push_back(pool.Create());
            }
            
            for (auto p : ptrs) {
                pool.Destroy(p);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = end - start;
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "FixedBlockPool: ", diff.count(), " ms\n");
        }

        // Unordered Map with new/delete (Pre mem manager)
        {
            auto start = std::chrono::high_resolution_clock::now();
            std::unordered_map<int, BenchmarkComponent*> map;
            map.reserve(ITERATIONS);
            
            for (int i = 0; i < ITERATIONS; ++i) {
                map[i] = new BenchmarkComponent();
            }
            
            for (int i = 0; i < ITERATIONS; ++i) {
                delete map[i];
                map.erase(i);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = end - start;
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "UnorderedMap + new/delete: ", diff.count(), " ms\n");
        }

        // Unordered Map with FixedBlockPool (Mem Manager)
        {
            FixedBlockPool<BenchmarkComponent, 1024> pool;
            pool.ReservePages(ITERATIONS / 1024 + 1);

            auto start = std::chrono::high_resolution_clock::now();
            std::unordered_map<int, BenchmarkComponent*> map;
            map.reserve(ITERATIONS);
            
            for (int i = 0; i < ITERATIONS; ++i) {
                map[i] = pool.Create();
            }
            
            for (int i = 0; i < ITERATIONS; ++i) {
                pool.Destroy(map[i]);
                map.erase(i);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = end - start;
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "UnorderedMap + FixedBlockPool: ", diff.count(), " ms\n");
        }
        
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "=================================================\n");
    }
}
