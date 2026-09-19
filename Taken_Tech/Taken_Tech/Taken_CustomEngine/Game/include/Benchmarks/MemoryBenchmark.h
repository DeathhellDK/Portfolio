#pragma once
/**
* @file      MemoryBenchmark.h
* @author    Jethro Sung
* @email     sung.h
* @date
*
* @brief   Implements the memory benchmark for comparing raw new/delete vs. FixedBlockPool performance.
*
* @version 1.0
*
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/


namespace Benchmarks {
    /*
    * @brief Runs a benchmark comparing raw new/delete against the FixedBlockPool memory manager for allocating BenchmarkComponent instances.
    */
    void RunMemoryBenchmark();
}
