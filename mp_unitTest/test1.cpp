#include <iostream>
#include <list>
#include <vector>

// 内存块类型
enum class BlockType {
    RISCV,
    X86
};

// 内存池类
class MemoryPool {
public:
    // 构造函数，传入大区块的指针和区块大小
    MemoryPool(void* memoryBlock, size_t blockSize, size_t riscvSize, size_t x86Size) 
        : memoryBlock_(memoryBlock), blockSize_(blockSize), riscvSize_(riscvSize), x86Size_(x86Size) {
        // init block
        freeList_.emplace_back(memoryBlock_, blockSize_);
    }

    // allocate
    void* allocate(BlockType type, size_t numInstructions) {
        size_t size = (type == BlockType::RISCV) ? riscvSize_ * numInstructions : x86Size_ * numInstructions;

        for (auto it = freeList_.begin(); it != freeList_.end(); ++it) {
            if (it->second >= size) {
                void* allocatedMemory = it->first;
                if (it->second > size) {
                    it->first = static_cast<char*>(it->first) + size;
                    it->second -= size;
                } else {
                    freeList_.erase(it);
                }
                return allocatedMemory;
            }
        }
        return nullptr;
    }

    // free
    void deallocate(void* ptr, size_t numInstructions, BlockType type) {
        size_t size = (type == BlockType::RISCV) ? riscvSize_ * numInstructions : x86Size_ * numInstructions;
        freeList_.emplace_back(ptr, size);
        mergeFreeBlocks();
    }

private:
    void mergeFreeBlocks() {
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

    void* memoryBlock_;
    size_t blockSize_;
    size_t riscvSize_;
    size_t x86Size_;
    std::list<std::pair<void*, size_t>> freeList_;
};

int main() {
    // 模拟大区块内存
    const size_t poolSize = 1024;
    std::vector<char> memoryBlock(poolSize);

    // 每条RISC-V指令4字节，每条x86指令16字节
    MemoryPool pool(memoryBlock.data(), poolSize, 4, 16);

    // 分配和释放示例
    void* riscvPtr1 = pool.allocate(BlockType::RISCV, 10); // 分配10条RISC-V指令的空间
    void* x86Ptr1 = pool.allocate(BlockType::X86, 5); // 分配5条x86指令的空间

    std::cout << "Allocated 10 RISC-V instructions at: " << riscvPtr1 << std::endl;
    std::cout << "Allocated 5 x86 instructions at: " << x86Ptr1 << std::endl;

    pool.deallocate(riscvPtr1, 10, BlockType::RISCV); // 释放10条RISC-V指令的空间
    pool.deallocate(x86Ptr1, 5, BlockType::X86); // 释放5条x86指令的空间

    void* x86Ptr2 = pool.allocate(BlockType::X86, 10); // 分配10条x86指令的空间
    std::cout << "Allocated 10 x86 instructions at: " << x86Ptr2 << std::endl;

    return 0;
}
