#ifndef LIBMEMPOOL_H_
#define LIBMEMPOOL_H_

#include<list>
#include<functional>

// �ڴ������
enum class BlockType {
    RISCV,
    X86
};

// �ڴ����
class MemoryPool {
protected:
    void* memoryBlock_;
    size_t blockSize_;
    size_t riscvSize_;
    size_t x86Size_;
    std::list<std::pair<void*, size_t>> freeList_;
    std::function<void* (size_t)> large_block_allocator_;
    std::function<void (void*)> large_block_deallocator_;
public:
    // ���캯��������������ָ��������С
    MemoryPool(void* memoryBlock, size_t blockSize, size_t riscvSize, size_t x86Size,
        const std::function<void* (size_t)>& _alloc = [](size_t sz_)->auto { return ::operator new(sz_);  },
        const std::function<void(void*)> & _dealloc = [](void* bk_)->void { ::operator delete(bk_); });
    void* allocate(BlockType type, size_t numInstructions);
    //free
    void deallocate(void* ptr, size_t numInstructions, BlockType type);
    ~MemoryPool();
protected:
    void mergeFreeBlocks();
};

#endif // !LIBMEMPOOL_H_
