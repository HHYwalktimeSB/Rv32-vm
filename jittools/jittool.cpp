#include "pch.h"
#include "jittool.h"
#include"../Rv32-vm/_cpu.h"

//function templates
#define Hex(_VAL_) ((char)0x##_VAL_##u)
#define Make2(_v1_eqe,_v2_eqe) {Hex(_v1_eqe),Hex(_v2_eqe)}
#define Make3(_v1_eqe,_v2_eqe, _v3_eqe) {Hex(_v1_eqe),Hex(_v2_eqe),Hex(_v3_eqe)}
#define PC_INC(_BUF_) write_buf(_BUF_, {Hex(83),Hex(83), Hex(80), Hex(0),Hex(0),Hex(0),Hex(4)})
#define INC_CYCLES(_BUF_) buf = write_buf(_BUF_, {Hex(48),Hex(ff), Hex(83), Hex(88),Hex(0),Hex(0),Hex(0)})

template<unsigned N>
inline char* __fastcall write_buf(char* buf, const char (&data)[N] ) {
    for (unsigned i = 0; i < N; ++i) 
        buf[i] = data[i];
    return buf + N;
}

char* __fastcall write_buf(char* buf, const char* data, unsigned N) {
    for (unsigned i = 0; i < N; ++i)
        buf[i] = data[i];
    return buf + N;
}

char* __fastcall write_buf(char* buf, unsigned val) {
    *reinterpret_cast<unsigned*>(buf) = val;
    return buf + 4;
}

inline char* make_alu_instruction(char* buf , Instruction ins) {
#ifndef ARG_PASS_BY_RBX
    buf = write_buf(buf, { (char)0x48u, (char)0x89u, Hex(CB) });
#endif
    if (ins.rType.rd == 0)goto func_end_epilog;
    if (ins.rType.rs1 == ins.rType.rd&&ins.rType.funct7 != 0b0100000 && (ins.rType.funct3 == 0 ||
        ins.rType.funct3 == 4|| ins.rType.funct3 == 6|| ins.rType.funct3 == 7) ) {
        if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
        switch (ins.rType.funct3) {
        case 0:
            buf = write_buf(buf, Make2(01, 43));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            break;
        case 4://xor
            buf = write_buf(buf, Make2(31, 43));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            break;
        case 6://or
            buf = write_buf(buf, Make2(09, 43));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            break;
        case 7:
            buf = write_buf(buf, Make2(21, 43));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            break;
        }
    }
    else {
        if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (unsigned char)(ins.rType.rs1 << 2), ++buf;
        switch (ins.rType.funct3)
        {
        case 0:
            if (ins.rType.funct7 == 0b0100000) {//sub
                if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(2b, 03));
                else buf = write_buf(buf, Make2(2b, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            }
            else {//add
                if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(03, 03));
                else buf = write_buf(buf, Make2(03, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            }
            break;
        case 1: //shift left logic
            if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 0b));
            else buf = write_buf(buf, Make2(8b, 4b)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            buf = write_buf(buf, Make2(d3, e0));
            break;
        case 2: //slt
        case 3: //sltu
            if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(3b, 03));
            else buf = write_buf(buf, Make2(3b, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;//cmp
            buf = write_buf(buf, Make3(31, c0, c3));// xor eax, eax
            if (ins.rType.funct3 == 2)buf = write_buf(buf, Make3(0f, 9c, c0)); else buf = write_buf(buf, Make3(0f, 92, c0));//set
            break;
        case 4://xor
            if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(33, 03));
            else buf = write_buf(buf, Make2(33, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            break;
        case 5://rs
            if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 0b));
            else buf = write_buf(buf, Make2(8b, 4b)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            if (ins.rType.funct7 == 0b0100000)buf = write_buf(buf, Make2(d3, f8));
            else buf = write_buf(buf, Make2(d3, e8));
            break;
        case 6://or
            if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(0b, 03));
            else buf = write_buf(buf, Make2(0b, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
            break;
        case 7://and
            if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(23, 03));
            else buf = write_buf(buf, Make2(23, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;
        }
        buf = write_buf(buf, Make2(89, 43)), * buf = (unsigned char)(ins.rType.rd << 2), ++buf;
    }
    func_end_epilog:
    buf = PC_INC(buf);
    INC_CYCLES(buf);
    return buf;
}

inline char* make_alu_instruction_imm(char* buf, Instruction ins) {
#ifndef ARG_PASS_BY_RBX
    buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
    unsigned int imm = _sign_ext<12>(ins.iType.imm);
    if (ins.rType.rd == 0)goto func_end_epilog;
    if (ins.rType.rs1 == ins.rType.rd && (ins.rType.funct3 == 0 ||
        ins.rType.funct3 == 4 || ins.rType.funct3 == 6 || ins.rType.funct3 == 7)) {
        switch (ins.rType.funct3) {
        case 0:
            buf = write_buf(buf, Make2(81, 43));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            buf = write_buf(buf, imm);
            break;
        case 4://xor
            buf = write_buf(buf, Make2(81, 73));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
             buf = write_buf(buf, imm);
            break;
        case 6://or
            buf = write_buf(buf, Make2(81, 63));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            buf = write_buf(buf, imm);
            break;
        case 7:
            buf = write_buf(buf, Make2(81, 4b));
            *buf = (char)(unsigned)(ins.rType.rs1 << 2);
            ++buf;
            buf = write_buf(buf, imm);
            break;
        }
    }
    else {
        if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (ins.rType.rs1 << 2), ++buf;
        switch (ins.rType.funct3)
        {
        case 0://add
            *buf = Hex(05), ++buf;
            buf = write_buf(buf, (char*)&imm, 4);
            break;
        case 1: //shift left logic
            *buf = Hex(b1), ++buf;
            *buf = (unsigned char)imm, ++buf;
            buf = write_buf(buf, Make2(d3, e0));
            break;
        case 2: //slt
        case 3: //sltu
            *buf = Hex(3d), ++buf; buf = write_buf(buf, imm);//cmp eax, imm
            buf = write_buf(buf, Make3(31, c0, c3));// xor eax,rax
            if (ins.rType.funct3 == 2)buf = write_buf(buf, Make3(0f, 9c, c0)); //set signed
            else buf = write_buf(buf, Make3(0f, 92, c0));//set unsigned
            break;
        case 4://xor
            *buf = Hex(35), ++buf;
            buf = write_buf(buf, (char*)&imm, 4);
            break;
        case 5://rs
            *buf = Hex(b1), ++buf;
            if (ins.rType.funct7 == 0b0100000) {
                ins.rType.funct7 = 0u;
                *buf = (unsigned char)ins.iType.imm, ++buf;
                buf = write_buf(buf, Make2(d3, f8));
            }
            else {
                *buf = (unsigned char)imm, ++buf;
                buf = write_buf(buf, Make2(d3, e8));
            }
            break;
        case 6://or
            *buf = Hex(0d), ++buf;
            buf = write_buf(buf, (char*)&imm, 4);
            break;
        case 7://and
            *buf = Hex(25), ++buf;
            buf = write_buf(buf, (char*)&imm, 4);
        }
        buf = write_buf(buf, Make2(89, 43)), * buf = (ins.rType.rd << 2), ++buf;
    }
func_end_epilog:
    buf = PC_INC(buf);
    INC_CYCLES(buf);
    return buf;
}

char* add_epilog_(char* buf, int rval) {
    if (rval == 0)buf = write_buf(buf, Make3(31, c0, c3));
    else {
        *buf = Hex(b8); ++buf;
        buf = write_buf(buf, rval);
        *buf = Hex(c3); ++buf;
    }
    return buf;
}

char* make_btype_ins(char* buf, Instruction ins) {
#ifndef ARG_PASS_BY_RBX
    buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
    unsigned int imm = 0;
    imm |= ((unsigned)(ins.bType.imm_4_1)) << 1;
    imm |= ((unsigned)(ins.bType.imm_10_5)) << 5;
    imm |= ((unsigned)(ins.bType.imm_11)) << 11;
    imm |= ((unsigned)(ins.bType.imm_12)) << 12;
    imm = _sign_ext<13>(imm) - 4;
    if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
    else buf = write_buf(buf, Make2(8b, 43)), * buf = (ins.bType.rs1 << 2), ++buf;
    if (ins.rType.rs2 == 0)buf = write_buf(buf, Make2(3b, 03));
    else buf = write_buf(buf, Make2(3b, 43)), * buf = (unsigned char)(ins.rType.rs2 << 2), ++buf;//cmp
    switch (ins.bType.funct3)
    {
    case 1://bne
        __asm sete cl;
        buf = write_buf(buf, Make3(0f, 94,c1));
        break;
    case 0://beq
        buf = write_buf(buf, Make3(0f, 95, c1));
        break;
    case 2:
    case 3:
        buf = add_epilog_(buf, RC_RRT_INV_INSTRUCTION);
        break;
    case 5://bge
        buf = write_buf(buf, Make3(0f, 9c, c1));//setl
        break;
    case 4://blt
        buf = write_buf(buf, Make3(0f, 9d, c1));//setge
        break;
    case 7://bgeu
        buf = write_buf(buf, Make3(0f, 92, c1));//setb
        break;
    case 6://bltu
        buf = write_buf(buf, Make3(0f, 93, c1));//setae
        break;
    }
    *buf = Hex(b8), ++buf; buf = write_buf(buf, imm);// MOV EAX, imm
    buf = write_buf(buf, {Hex(c0),Hex(e1),Hex(05)       ,Hex(48), Hex(d3),Hex(e0),       Hex(83), Hex(C0), Hex(04),     
        Hex(01), Hex(83), Hex(80) , Hex(0),Hex(0), Hex(0)});//shl cl,5; shl rax cl; add eax, 4; add [rbx+80], eax;
    INC_CYCLES(buf);
    buf = add_epilog_(buf, 0);
    return buf;
}

#define LOAD_EAX_PTR_OFF_L(_OFFSET_) buf = write_buf(buf, Make2(8B, 83)); buf = write_buf(buf, _OFFSET_)
#define STORE_EAX_PTR_OFF_L(_OFFSET_) buf = write_buf(buf, Make2(89, 83)); buf = write_buf(buf, _OFFSET_)
#define ADD_EAX_IMM(_IMM_fuck) *buf = Hex(05), ++buf;\
buf = write_buf(buf, _IMM_fuck)

__declspec(dllexport) unsigned long long __fastcall call_my_fn(const void* pfn, void* preg)
{
    unsigned long long ret;
    __asm {
        mov rbx, preg
        mov rdx, rsp
        call pfn
        mov ret, rax
    }
    return ret;
}

MY_API unsigned c_instruction(unsigned int eeeeins, char* buf)
{
    char* be = buf;
    Instruction ins;
    *reinterpret_cast<unsigned int*>(&ins) = eeeeins;
    switch (ins.rType.opcode)
    {
    case OP_LUI:
#ifndef ARG_PASS_BY_RBX
        buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
        eeeeins = ((unsigned)ins.uType.imm_31_12) << 12;
        *buf = Hex(b8), ++buf; buf = write_buf(buf, (char*)&eeeeins, 4);// MOV EAX, ...
        if (ins.rType.rd != 0)buf = write_buf(buf, Make2(89, 43)), * buf = (ins.rType.rd << 2), ++buf;
        buf = PC_INC(buf);
        INC_CYCLES(buf);
        break;
    case OP_AUIPC:
#ifndef ARG_PASS_BY_RBX
        buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
        eeeeins = 32 <<2;
        buf = write_buf(buf, Make2(8B, 83));
        buf = write_buf(buf, eeeeins);
        *buf = Hex(05), ++buf;
        buf = write_buf(buf, eeeeins);
        if (ins.rType.rd != 0)buf = write_buf(buf, Make2(89, 43)), * buf = (ins.rType.rd << 2), ++buf;
        buf = PC_INC(buf);
        INC_CYCLES(buf);
        break;
    case OP_JAL:
#ifndef ARG_PASS_BY_RBX
        buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
        LOAD_EAX_PTR_OFF_L(128);
        buf = write_buf(buf, { Hex(83), Hex(C0), Hex(04) });
        if (ins.rType.rd != 0)buf = write_buf(buf, Make2(89, 43)), * buf = (ins.rType.rd << 2), ++buf;
        eeeeins = ((unsigned)ins.jType.imm_10_1) << 1;
        eeeeins |= ((unsigned)ins.jType.imm_11) << 11;
        eeeeins |= ((unsigned)ins.jType.imm_19_12) << 12;
        eeeeins |= ((unsigned)ins.jType.imm_20) << 20;
        eeeeins -= 4;
        if (eeeeins != 0)ADD_EAX_IMM(eeeeins);
        STORE_EAX_PTR_OFF_L(128);
        INC_CYCLES(buf);
        buf = add_epilog_(buf, 0);
        break;
    case OP_JALR:
#ifndef ARG_PASS_BY_RBX
        buf = write_buf(buf, { (char)0x48, (char)0x89, Hex(CB) });
#endif
        LOAD_EAX_PTR_OFF_L(128);
        buf = write_buf(buf, { Hex(83), Hex(C0), Hex(04) });
        if (ins.rType.rd != 0)buf = write_buf(buf, Make2(89, 43)), * buf = (ins.rType.rd << 2), ++buf;
        //move new val
        if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (unsigned char)(ins.rType.rs1 << 2), ++buf;
        eeeeins = _sign_ext<12>(ins.iType.imm) - 4;
        if(eeeeins !=0)ADD_EAX_IMM(eeeeins);
        STORE_EAX_PTR_OFF_L(128);
        INC_CYCLES(buf);
        buf = add_epilog_(buf, 0);
        break;
    case OP_BTYPE:
        buf = make_btype_ins(buf, ins);
        break;
    case OP_ALU:
        buf = make_alu_instruction(buf, ins);
        break;
    case OP_ALU_IMM:
        buf = make_alu_instruction_imm(buf, ins);
        break;
    case OP_STORE:
        if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (unsigned char)(ins.rType.rs1 << 2), ++buf;
        eeeeins = ((unsigned)ins.sType.imm_4_0); eeeeins |= ((unsigned)ins.sType.imm_11_5) << 5;
        eeeeins = _sign_ext<12>(eeeeins);
        if (eeeeins != 0) {
            *buf = (char)0x05; ++buf;
            buf = write_buf(buf, eeeeins);
        }
        *buf = (char)0xb9; ++buf;
        eeeeins = ins.sType.rs2; eeeeins <<= 27; eeeeins |= 2;
        switch (ins.rType.funct3)
        {
        case 0:
            eeeeins |= (0x1u << 24);
            break;
        case 1:
            eeeeins |= (2u << 24);
            break;
        case 2:
            eeeeins |= (4u << 24);
            break;
        default:
            buf = add_epilog_(buf, RC_RRT_INV_INSTRUCTION);
            break;
        }
        buf = write_buf(buf, eeeeins);
        //buf = PC_INC(buf);
        buf = write_buf(buf, { Hex(48), Hex(c1),Hex(e0),Hex(20) });
        buf = write_buf(buf, Make3(48, 09, c8));
        INC_CYCLES(buf);
        *buf = (char)0xc3; ++buf;
        break;
    case OP_SYSTEM:
        buf = add_epilog_(buf, 3);
        break;
    case OP_LOAD:
        if (ins.rType.rs1 == 0)buf = write_buf(buf, Make2(8b, 03));
        else buf = write_buf(buf, Make2(8b, 43)), * buf = (unsigned char)(ins.rType.rs1 << 2), ++buf;
        eeeeins = _sign_ext<12>(ins.iType.imm);
        if (eeeeins != 0) {
            buf[0] = (char)0x05; ++buf;
            buf = write_buf(buf, eeeeins);
        }
        buf = write_buf(buf, { Hex(48), Hex(c1),Hex(e0),Hex(20) });
        buf[0] = (char)0xb9; ++buf;
        eeeeins = ins.rType.rd; eeeeins <<= 27; eeeeins |= 1;
        switch (ins.rType.funct3)
        {
        case 0:
            eeeeins |= (0x1u << 24);
            eeeeins |= (1 << 22);
            break;
        case 1:
            eeeeins |= (2u << 24);
            eeeeins |= (2 << 22);
            break;
        case 2:
            eeeeins |= (4u << 24);
            break;
        case 4:
            eeeeins |= (1u << 24);
            break;
        case 5:
            eeeeins |= (2u << 24);
            break;
        default:
            buf = add_epilog_(buf, RC_RRT_INV_INSTRUCTION);
            break;
        }
        buf = write_buf(buf, eeeeins);
        //buf = PC_INC(buf);
        buf = write_buf(buf, Make3(48, 09, c8));
        INC_CYCLES(buf);
        buf[0] = (char)0xc3; ++buf;
        break;
    default:
        buf = add_epilog_(buf, 4);
        //rrc = 4;
        break;
    }
    return buf - be;
}

int MambaCache_::getHeight(Node* n)
{
    if (n == NULL)
        return 0;
    return n->height;
}

MambaCache_::Node* MambaCache_::createNode(int key, MambaEntry_* e)
{
    MambaCache_::Node* node = new Node;
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    node->data = e;
    return node;
}

int MambaCache_::getBalanceFactor(Node* n)
{
    if (n == NULL) {
        return 0;
    }
    return getHeight(n->left) - getHeight(n->right);
}

MambaCache_::Node* MambaCache_::rightRotate(Node* y)
{
    Node* x = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left = T2;

    x->height = max(getHeight(x->right), getHeight(x->left)) + 1;
    y->height = max(getHeight(y->right), getHeight(y->left)) + 1;

    return x;
}

MambaCache_::Node* MambaCache_::leftRotate(Node* x)
{
    struct Node* y = x->right;
    struct Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(getHeight(x->right), getHeight(x->left)) + 1;
    y->height = max(getHeight(y->right), getHeight(y->left)) + 1;

    return y;
}

MambaCache_::Node* MambaCache_::insert(Node* node, int key, MambaEntry_* e)
{
    if (node == NULL)
        return  createNode(key, e);

    if (key < node->key)
        node->left = insert(node->left, key, e);
    else if (key > node->key)
        node->right = insert(node->right, key, e);

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    int bf = getBalanceFactor(node);

    // Left Left Case  
    if (bf > 1 && key < node->left->key) {
        return rightRotate(node);
    }
    // Right Right Case  
    if (bf<-1 && key > node->right->key) {
        return leftRotate(node);
    }
    // Left Right Case  
    if (bf > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    // Right Left Case  
    if (bf < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

MambaCache_::Node* __fastcall MambaCache_::find(int key, Node* cur)
{
    if(!cur)return nullptr;
    if (cur->key == key)return cur;
    if (key < cur->key)return find(key, cur->left);
    return find(key, cur->right);
}

void MambaEntry_::construct_buf()
{
    buf = (char*)VirtualAlloc(NULL, mambaEXEbuf_reserved_SZ, MEM_RESERVE | MEM_COMMIT,
        PAGE_READONLY);
}

void MambaEntry_::deconstruct_buf()
{
    VirtualFree(buf, mambaEXEbuf_reserved_SZ, MEM_RELEASE);
}

void MambaEntry_::compile_1(unsigned* pmem, unsigned addr)
{
    addr >>= 6;
    addr &= 3;
    char* ins_stat = buf + mambaEXEbuf_reserved_SZ / 4 * addr;
    unsigned i = 0;
    DWORD cr;
    VirtualProtect(buf, mambaEXEbuf_reserved_SZ, PAGE_READWRITE, &cr);
func_start:
    __try {
        for (; i < mambaCACHELINE_SZ / 4; ++i) {
            fptr[i + addr*16] = ins_stat;
            ins_stat += c_instruction(pmem[i], ins_stat);
        }
        add_epilog_(ins_stat, 0);
        this->description[addr].V = 1;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER :
        EXCEPTION_CONTINUE_SEARCH) {
        VirtualAlloc(reinterpret_cast<void*>((reinterpret_cast<unsigned long long>(pmem + i)&0xFFFFFFFFFFFFF000)),
            4096, MEM_COMMIT, PAGE_READWRITE);
    }
    if (i != 16)goto func_start;
    VirtualProtect(buf, mambaEXEbuf_reserved_SZ, PAGE_EXECUTE, &cr);
}

const void* __fastcall MambaEntry_::read(unsigned addr)
{
    addr >>= 2;
    if (description[(addr >> 4) & 3].V == 0) compile_1(binding_pmeml,  addr<<2);
    description[(addr >> 4) & 3].C += 1;
    return fptr[(addr & 15) + ((addr >> 4) & 3)*16];
}

void MambaEntry_::flush()
{
    for (unsigned i = 0; i < 4; ++i) {
        *reinterpret_cast<unsigned*>(description + i) = 0;
    }
}

void MambaEntry_::write(unsigned addr)
{
    addr >>= 6;
    addr &= 3;
    this->description[addr].V = 0;
}

unsigned MambaEntry_::cnt_all() const
{
    unsigned r = 0;
    for (unsigned i = 0; i < 4; ++i)if (description[i].V)r += description[i].C;
    return r;
}

void MambaEntry_::bind(unsigned* pmem, unsigned ma)
{
    binding_pmeml = pmem;
    mem_mask = ma;
}

MambaEntry_::MambaEntry_()
{
    for (unsigned i = 0; i < 4; ++i) {
        description[i].V = 0;
    }
    construct_buf();
}

MambaEntry_::MambaEntry_(unsigned* base_ptr, unsigned ma)
{
    for (unsigned i = 0; i < 4; ++i) {
        description[i].V = 0;
    }
    construct_buf();
    bind(base_ptr, ma);
}

MambaEntry_::~MambaEntry_()
{
    deconstruct_buf();
}

MambaCache_::Node* MambaCache_::deleteNode(Node* root, int key) {
    // STEP 1: PERFORM STANDARD BST DELETE
    if (root == nullptr)
        return root;

    // If the key to be deleted is smaller 
    // than the root's key, then it lies in 
    // left subtree
    if (key < root->key)
        root->left = deleteNode(root->left, key);

    // If the key to be deleted is greater 
    // than the root's key, then it lies in 
    // right subtree
    else if (key > root->key)
        root->right = deleteNode(root->right, key);

    // if key is same as root's key, then 
    // this is the node to be deleted
    else {
        // node with only one child or no child
        if ((root->left == nullptr) ||
            (root->right == nullptr)) {
            Node* temp = root->left ?
                root->left : root->right;

            // No child case
            if (temp == nullptr) {
                temp = root;
                root = nullptr;
            }
            else // One child case
                *root = *temp; // Copy the contents of 
            // the non-empty child
            free(temp);
        }
        else {
            // node with two children: Get the 
            // inorder successor (smallest in 
            // the right subtree)
            Node* temp = minValueNode(root->right);

            // Copy the inorder successor's 
            // data to this node
            root->key = temp->key;

            // Delete the inorder successor
            root->right = deleteNode(root->right, temp->key);
        }
    }

    // If the tree had only one node then return
    if (root == nullptr)
        return root;

    // STEP 2: UPDATE HEIGHT OF THE CURRENT NODE
    root->height = 1 + max(getHeight(root->left),
        getHeight(root->right));

    // STEP 3: GET THE BALANCE FACTOR OF THIS 
    // NODE (to check whether this node 
    // became unbalanced)
    int balance = getBalanceFactor(root);

    // If this node becomes unbalanced, then 
    // there are 4 cases

    // Left Left Case
    if (balance > 1 &&
        getBalanceFactor(root->left) >= 0)
        return rightRotate(root);

    // Left Right Case
    if (balance > 1 &&
        getBalanceFactor(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // Right Right Case
    if (balance < -1 &&
        getBalanceFactor(root->right) <= 0)
        return leftRotate(root);

    // Right Left Case
    if (balance < -1 &&
        getBalanceFactor(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

MambaCache_::Node* MambaCache_::minValueNode(Node* node)
{
    if(node == nullptr)return nullptr;
    if (node->left == nullptr)return node;
    return minValueNode(node->left);
}

unsigned MambaCache_::HashFn_FNV1a(unsigned val)
{
    unsigned hash = 0x811c9dc5;
    hash ^= ((unsigned char*)(&val))[0];
    hash *= 0x01000193;
    hash ^= ((unsigned char*)(&val))[1];
    hash *= 0x01000193;
    hash ^= ((unsigned char*)(&val))[2];
    hash *= 0x01000193;
    hash ^= ((unsigned char*)(&val))[3];
    hash *= 0x01000193;
    return hash;
}

void fuck(MambaCache_::Node* cur, MambaCache_::Node*& out, int& v) {
    int x = cur->data->cnt_all();
    if (x > v) {
        v = x;
        out = cur;
    }
}

MambaEntry_* MambaCache_::mambaout(unsigned index)
{
    MambaCache_::Node* out = table[index];
    int ox = table[index]->data->cnt_all();
    InorderRecursiveHelper(table[index], &fuck, out, ox);
    auto ret = out->data;
    table[index] = deleteNode(table[index], out->key);
    return ret;
}

void MambaCache_::add(unsigned addr, unsigned index)
{
    addr &= 0xffffff00;
    if (table[index]&& cnt_nds[index] == 7) {
        auto m = mambaout(index);
        m->flush();
        m->bind((unsigned*)(base + addr),mam_mask);
        table[index] = insert(table[index], addr, m);
    }
    else {
        table[index] = insert(table[index], addr, new MambaEntry_((unsigned*)(base + addr),mam_mask));
        cnt_nds[index] += 1;
    }
}

void MambaCache_::bind(char* addrbase)
{
    base = addrbase;
}

const void* MambaCache_::read(unsigned addr)
{
    unsigned i = HashFn_FNV1a(addr & 0xffffff00) % 512;
    if (table[i] == nullptr)add(addr, i);
    return find(addr & 0xffffff00, table[i])->data->read(addr);
}

MambaCache_::MambaCache_(char* base, unsigned ma) {
    for (unsigned i = 0; i < 512; ++i)table[i] = nullptr;
    bind(base);
    this->mam_mask = ma;
}

const void* MambaCache_::read_withoutHAJIfunction(unsigned addr)
{
    unsigned i = (addr >> 8) % 512;
    auto x = find(addr & 0xffffff00, table[i]);
    if (x == nullptr)add(addr, i), x = find(addr & 0xffffff00, table[i]);
    i = (addr >> 6) & 3;
    if (x->data->description[i].V == 0)x->data->compile_1((unsigned*)(base + (addr & 0xffffffc0)), addr & 0xffffffc0);
    return x->data->fptr[((addr>>2)&15) + i*16];
}

__declspec(naked) void __fastcall ddddd(void* p) {
    __asm {

        add eax, 2047
        shl rax, 32
        mov ecx, 0xf8300001
        or rax, rcx
        inc qword ptr[rbx+136]
        ret
    }
}

MY_API void call_test_fn() {
    REGS reg { 0 };
    reg.x[1] = 5;
    reg.x[2] = 1;
    auto x = call_my_fn(&ddddd, &reg);
    x >>= 32;
    reg.pc += x;
    return;
}