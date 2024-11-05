#ifndef _JITTOOLS_H_
#define _JITTOOLS_H_

//compil with icc

#ifdef _DLL
#define MY_API extern "C" __declspec(dllexport)
#define MY_API_V2 __declspec(dllexport)
#else 
#ifndef MY_API 
#define MY_API extern "C"
#define MY_API_V2 
#endif
#endif // _DLL

struct _MyFnRetStruct {
    unsigned vaddr;
    unsigned REG : 5;
    unsigned F3 : 3;
    unsigned SB : 2;//sign_bytes
    unsigned EC : 21;//ecode
};

//63:32 vaddr, 32:27 io_regret, 26:0 error code
MY_API_V2 unsigned long long __fastcall call_my_fn(const void* pfn, void* preg);

MY_API unsigned c_instruction(unsigned int ins, char* buf);

MY_API void call_test_fn();

#include<utility>

#define ARG_PASS_BY_RBX

#define RC_RRT_OK 0
#define RC_RRT_MEMLOAD 1
#define RC_RRT_MEMSTORE 2
#define RC_RRT_SYSCALL 3
#define RC_RRT_INV_INSTRUCTION 4

class MambaEntry_ {
public:
    constexpr static unsigned int mambaCACHELINE_SZ = 64;
    constexpr static unsigned int mambaEXEbuf_reserved_SZ = mambaCACHELINE_SZ* 64;
    struct MyAddrstruct {
        unsigned tag : 24;
        unsigned seek_1 : 2;
        unsigned offset : 4;
        unsigned unused : 2;
    };
    struct LineDescription
    {
        unsigned V : 1;
        unsigned A : 1;
        unsigned C : 30;
        constexpr inline LineDescription():V(0), A(0), C(0) { }
    };
private:
    const void* fptr[mambaCACHELINE_SZ];
    LineDescription description[4];
    char* buf;
    unsigned* binding_pmeml;
    unsigned mem_mask;
    void construct_buf();
    void deconstruct_buf();
    //void vfprotect_set(unsigned vflags);
public:
    void compile_1(unsigned* pmem, unsigned addr);
    const void*  __fastcall read(unsigned addr);
    void flush();
    void write(unsigned addr);
    unsigned cnt_all()const;
    void bind(unsigned* pmem, unsigned ma);
    MambaEntry_();
    MambaEntry_(unsigned* base_ptr, unsigned ma);
    ~MambaEntry_();
    friend class MambaCache_;
};

class MambaCache_ {
public:
    struct Node
    {
        int key;
        int height;
        MambaEntry_* data;
        Node* left;
        Node* right;
    };
protected:
    Node* table[512];
    int cnt_nds[512];
    char* base;
    unsigned mam_mask;
    int getHeight(struct Node* n);
    Node* createNode(int key, MambaEntry_* e);
    int getBalanceFactor(struct Node* n);
    Node* rightRotate(struct Node* y);
    Node* leftRotate(struct Node* x);
    Node* insert(struct Node* node, int key, MambaEntry_* e);
    static Node* __fastcall find(int key, Node* cur);
    Node* deleteNode(Node* root, int key);
    Node* minValueNode(Node* node);
// avlend
    static unsigned __fastcall HashFn_FNV1a(unsigned val);
    template<class... arg>
    void InorderRecursiveHelper(Node* rt, void(*_f)(Node*, arg...), arg&&... args) {
        if (rt == NULL)return;
        InorderRecursiveHelper(rt->left, _f, std::forward<arg>(args)...);
        _f(rt, std::forward<arg>(args)...);
        InorderRecursiveHelper(rt->right, _f, std::forward<arg>(args)...);
    }
    MambaEntry_* mambaout(unsigned index);
public:
    void add(unsigned addr, unsigned index);
    void bind(char* addrbase);
    __declspec(dllexport)
        const void* read(unsigned addr);
    __declspec(dllexport) 
        MambaCache_(char* base, unsigned ma);
    __declspec(dllexport)
        const void* read_withoutHAJIfunction(unsigned addr);
};

#endif // !_JITTOOLS_H_