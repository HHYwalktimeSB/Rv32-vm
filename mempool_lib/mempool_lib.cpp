#include <iostream>
#include <list>
#include <vector>

#include"mempool_lib.h"

MemoryPool::MemoryPool(void* memoryBlock, size_t blockSize, size_t riscvSize, size_t x86Size, 
    const std::function<void* (size_t)>& _alloc , const std::function<void(void*)> & _dealloc)
        : memoryBlock_(memoryBlock), blockSize_(blockSize), riscvSize_(riscvSize), x86Size_(x86Size),
 large_block_allocator_(_alloc), large_block_deallocator_(_dealloc){
        // init block
        freeList_.emplace_back(memoryBlock_, blockSize_);
    }

    // allocate
    void* MemoryPool::allocate(BlockType type, size_t numInstructions) {
        size_t size = (type == BlockType::RISCV) ? riscvSize_ * numInstructions : x86Size_ * numInstructions;

        for (auto it = freeList_.begin(); it != freeList_.end(); ++it) {
            if (it->second >= size) {
                void* allocatedMemory = it->first;
                if (it->second > size) {
                    it->first = static_cast<char*>(it->first) + size;
                    it->second -= size;
                }
                else {
                    freeList_.erase(it);
                }
                return allocatedMemory;
            }
        }
        return nullptr;
    }
    void MemoryPool:: deallocate(void* ptr, size_t numInstructions, BlockType type) {
        size_t size = (type == BlockType::RISCV) ? riscvSize_ * numInstructions : x86Size_ * numInstructions;
        freeList_.emplace_back(ptr, size);
        mergeFreeBlocks();
    }

    MemoryPool::~MemoryPool()
    {
        this->large_block_deallocator_(this->memoryBlock_);
    }

    void MemoryPool::mergeFreeBlocks() {
        freeList_.sort([](const std::pair<void*, size_t>& a, const std::pair<void*, size_t>& b) {
            return a.first < b.first;
            });
        for (auto it = freeList_.begin(); it != freeList_.end(); ++it) {
            auto next = std::next(it);
            if (next != freeList_.end() && static_cast<char*>(it->first) + it->second == next->first) {
                it->second += next->second;
                freeList_.erase(next);
            }
        }
    }