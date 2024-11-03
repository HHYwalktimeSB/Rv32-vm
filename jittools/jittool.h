#ifndef _JITTOOLS_H_
#define _JITTOOLS_H_

//compil with icc

#ifdef _DLL
#define MY_API extern "C" __declspec(dllexport)
#else 
#ifndef MY_API 
#define MY_API extern "C"
#endif
#endif // _DLL

//return 0 for no need after processing, return 1 for memory load op, return 2 for mem write, 3 for syscall
MY_API int __fastcall call_my_fn(const void* pfn, void* preg);

MY_API unsigned c_instruction(unsigned int ins, char* buf, unsigned& rrc);

MY_API void call_test_fn();

#include<utility>

#define ARG_PASS_BY_RBX

#define RC_RRT_OK 0
#define RC_RRT_MEMLOAD 1
#define RC_RRT_MEMSTORE 2
#define RC_RRT_SYSCALL 3
#define RC_RRT_INV_INSTRUCTION 4

MY_API const void* _getfptr001(int rc);

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
    const void* fptr[4][mambaCACHELINE_SZ/4];
    LineDescription description[4];
    char* buf;
    unsigned* binding_pmeml;
    inline const void* getVaddr(unsigned addr)const {
        addr >>= 2;
        return fptr[(addr>>4)&3][addr&15];
    }
    void construct_buf();
    void deconstruct_buf();
    void vfprotect_set(unsigned vflags);
public:
    void compile_1(unsigned* pmem, unsigned addr);
    const void*  __fastcall read(unsigned addr);
    void flush();
    void write(unsigned addr);
    unsigned cnt_all()const;
    void bind(unsigned* pmem);
    MambaEntry_();
    MambaEntry_(unsigned* base_ptr);
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
    __declspec(dllexport) const void* read(unsigned addr);
    __declspec(dllexport) MambaCache_(char* base);
    __declspec(dllexport) const void* read_withoutHAJIfunction(unsigned addr);
};

#endif // !_JITTOOLS_H_