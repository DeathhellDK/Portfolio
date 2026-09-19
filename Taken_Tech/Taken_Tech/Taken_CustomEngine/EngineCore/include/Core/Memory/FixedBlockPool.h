#pragma once
/**
 * @file    FixedBlockPool.h
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2026-02-02
 *
 * @brief   Page-based free-list alloc for gameplay objects/components.
 *
 *   - Fixed-size blocks for type T
 *   - Pages (each page contains N blocks)
 *   - Free linked list for O(1) alloc/free
 *   - Placement-new construction + explicit stor
 *   - No STL containers used inside allocator
 *   - Runtime expansion by adding new pages when exhausted
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <cstddef>   // std::size_t
#include <new>       // placement new, ::operator new/delete
#include <cassert>   // assert
#include <utility>   // std::forward

namespace eng::mem
{
    /**
     * @class FixedBlockPool
     * @brief A fixed-size block allocator using a free list and page expansion.
     * 
     * This pool allocates objects of type T in pages. Each page contains a fixed number
     * of blocks. It uses a free list for O(1) allocation and deallocation.
     * 
     * @tparam T The type of object to allocate.
     * @tparam BlocksPerPage The number of blocks per page. Defaults to 256.
     */
    template <typename T, std::size_t BlocksPerPage = 256>
    class FixedBlockPool
    {
    public:
        /**
         * @brief Default constructor.
         */
        FixedBlockPool() = default;
        FixedBlockPool(const FixedBlockPool&) = delete;
        
        /** @brief Deleted assignment operator to prevent copying. */
        FixedBlockPool& operator=(const FixedBlockPool&) = delete;

        /**
         * @brief Destructor. Releases all allocated pages.
         */
        ~FixedBlockPool()
        {
            // game must destroy all live objects before shutdown.
            ReleaseAllPages();
        }

        /**
         * @brief Pre-allocates a number of pages.
         * 
         * Useful for reserving memory at startup to avoid runtime allocations.
         * 
         * @param pageCount The number of pages to allocate.
         */
        void ReservePages(std::size_t pageCount)
        {
            for (std::size_t i = 0; i < pageCount; ++i)
                AddPage(/*countExpansion=*/false);
        }

        /**
         * @brief Constructs an object of type T in the pool.
         * 
         * Allocates memory from the pool and uses placement new to construct the object.
         * 
         * @tparam Args Argument types for the constructor.
         * @param args Arguments to forward to the constructor.
         * @return Pointer to the constructed object.
         */
        template <typename... Args>
        T* Create(Args&&... args)
        {
            void* mem = AllocateBlock();
            return new (mem) T(std::forward<Args>(args)...);
        }

        /**
         * @brief Destroys an object and returns its memory to the pool.
         * 
         * Calls the destructor of the object and adds the block back to the free list.
         * 
         * @param obj Pointer to the object to destroy.
         */
        void Destroy(T* obj)
        {
            if (!obj) return;
            obj->~T();
            FreeBlock(obj);
        }

        /**
         * @brief Allocates raw memory from the pool.
         * 
         * @return Pointer to the allocated memory block.
         */
        void* AllocateRaw()
        {
            return AllocateBlock();
        }

        /**
         * @brief Returns raw memory to the pool.
         * 
         * @param p Pointer to the memory block to free.
         */
        void FreeRaw(void* p)
        {
            if (!p) return;
            FreeBlock(p);
        }

        // --- Debug / proof-of-working counters ---
        /** @brief Returns the total number of pages allocated. */
        std::size_t PagesAllocated() const { return pagesAllocated_; }
        /** @brief Returns the number of currently live objects. */
        std::size_t LiveObjects()    const { return liveObjects_; }
        /** @brief Returns the number of times the pool expanded at runtime. */
        std::size_t Expansions()     const { return expansions_; }

    private:
        struct FreeNode { FreeNode* next; };

        static constexpr std::size_t BlockSize()
        {
            // Ensure each block can hold either T or a FreeNode pointer.
            return (sizeof(T) > sizeof(FreeNode)) ? sizeof(T) : sizeof(FreeNode);
        }

        struct Page
        {
            Page* next;
            alignas(T) unsigned char data[BlocksPerPage * BlockSize()];
        };

        /**
         * @brief Internal function to allocate a block.
         * 
         * pops from the free list. If empty, allocates a new page.
         * 
         * @return Pointer to the allocated block.
         */
        void* AllocateBlock()
        {
            if (!freeList_)
                AddPage(/*countExpansion=*/true);

            assert(freeList_ && "FixedBlockPool: out of memory and page allocation failed");
            FreeNode* n = freeList_;
            freeList_ = freeList_->next;

            ++liveObjects_;
            return n;
        }

        /**
         * @brief Internal function to free a block.
         * 
         * Pushes the block onto the free list.
         * 
         * @param p Pointer to the block to free.
         */
        void FreeBlock(void* p)
        {
            FreeNode* n = static_cast<FreeNode*>(p);
            n->next = freeList_;
            freeList_ = n;

            assert(liveObjects_ > 0 && "FixedBlockPool: double free or corruption");
            --liveObjects_;
        }

        /**
         * @brief Allocates a new page of blocks.
         * 
         * @param countExpansion Whether to count this as a runtime expansion.
         */
        void AddPage(bool countExpansion)
        {
            // OS alloc is permitted at load, and at runtime only when exhausted.
            Page* page = static_cast<Page*>(::operator new(sizeof(Page)));
            page->next = pages_;
            pages_ = page;

            ++pagesAllocated_;
            if (countExpansion) ++expansions_;

            // Carve blocks and push into free list.
            unsigned char* start = page->data;
            for (std::size_t i = 0; i < BlocksPerPage; ++i)
            {
                auto* node = reinterpret_cast<FreeNode*>(start + i * BlockSize());
                node->next = freeList_;
                freeList_ = node;
            }
        }

        /**
         * @brief Releases all allocated pages to the OS.
         */
        void ReleaseAllPages()
        {
            Page* p = pages_;
            while (p)
            {
                Page* next = p->next;
                ::operator delete(p);
                p = next;
            }
            pages_ = nullptr;
            freeList_ = nullptr;
        }

    private:
        Page* pages_ = nullptr;
        FreeNode* freeList_ = nullptr;

        std::size_t pagesAllocated_ = 0;
        std::size_t liveObjects_ = 0;
        std::size_t expansions_ = 0;
    };
} 