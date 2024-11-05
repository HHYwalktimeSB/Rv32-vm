#include"../Rv32-vm/_cpu.h"
#include<string>
#include<fstream>
#include<map>
#include<unordered_map>
#include<iomanip>
#include "rv32masm.h"

using tag_table = std::map< std::string, unsigned int >;
std::pair < tag_table, std::string > _internal_id_table[16];
std::unordered_map < std::string, void (*)(Instruction&)> opcode_t;

using namespace std;

constexpr int inv_instruction = - 1;
constexpr int str_no_para = 1;
constexpr int missing_para = -2;
constexpr int inv_regid = -3;
constexpr int inv_id = -4;

template<unsigned N>
constexpr unsigned make_bits() {
    static_assert(N <= 32, "bits no more than 32bits!");
    unsigned int ret = 0xffffffff;
    ret >>= 32 - N;
    return ret;
}

void to_upper(string& str) {
    for (auto a = str.begin(); a != str.end(); ++a) {
        if ('a' <= *a && *a <= 'z')
            *a = *a - 'a' + 'A';
    }
}

void op_make_lui(Instruction& ins) {
    ins.uType.opcode |= OP_LUI;
}

void op_make_auipc(Instruction& ins) {
    ins.uType.opcode |= OP_AUIPC;
}

void op_make_jal(Instruction& ins) {
    ins.jType.opcode |= OP_JAL;
}

void op_make_jalr(Instruction& ins) {
    ins.iType.opcode |= OP_JALR;
}

void op_make_beq(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
}

void op_make_bne(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
    ins.bType.funct3 |= 0b001;
}

void op_make_blt(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
    ins.bType.funct3 |= 0b100;
}

void op_make_bge(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
    ins.bType.funct3 |= 0b101;
}

void op_make_bltu(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
    ins.bType.funct3 |= 0b110;
}

void op_make_bgeu(Instruction& ins) {
    ins.bType.opcode |= OP_BTYPE;
    ins.bType.funct3 |= 0b111;
}

void op_make_lb(Instruction& ins) {
    ins.iType.opcode |= OP_LOAD;
}

void op_make_lh(Instruction& ins) {
    ins.iType.opcode |= OP_LOAD;
    ins.iType.funct3 |= 1;
}

void op_make_lw(Instruction& ins) {
    ins.iType.opcode |= OP_LOAD;
    ins.iType.funct3 |=2;
}

void op_make_lbu(Instruction& ins) {
    ins.iType.opcode |= OP_LOAD;
    ins.iType.funct3 |= 4;
}

void op_make_lhu(Instruction& ins) {
    ins.iType.opcode |= OP_LOAD;
    ins.iType.funct3 |= 5;
}

void op_make_sb(Instruction& ins) {
    ins.sType.opcode |= OP_STORE;
}

void op_make_sh(Instruction& ins) {
    ins.sType.opcode |= OP_STORE;
    ins.sType.funct3 |= 1;
}

void op_make_sw(Instruction& ins) {
    ins.sType.opcode |= OP_STORE;
    ins.sType.funct3 |= 2;
}

void op_make_addi(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
}

void op_make_slti(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b010;
}

void op_make_sltiu(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b011;
}

void op_make_xori(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b100;
}

void op_make_ori(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b110;
}

void op_make_andi(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM  ;
    ins.iType.funct3 |= 7;
}

void op_make_slli(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b001;
}

void op_make_srli(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b101u;
}

void op_make_srai(Instruction& ins) {
    ins.iType.opcode |= OP_ALU_IMM;
    ins.iType.funct3 |= 0b101;
    ins.rType.funct7 |= 0b0100000;
}

void op_make_add(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
}

void op_make_sub(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct7 |= 0b0100000;
}

void op_make_sll(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 1;
}

void op_make_slt(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 2;
}

void op_make_sltu(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 3;
}

void op_make_xor(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 4;
}

void op_make_srl(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 5;
}

void op_make_sra(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 5;
    ins.rType.funct7 |= 0b0100000;
}

void op_make_or(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 6;
}

void op_make_and(Instruction& ins) {
    ins.rType.opcode |= OP_ALU;
    ins.rType.funct3 |= 7;
}

bool cmppartstr_eq(const std::string& str, unsigned int index, unsigned int length, const char* b) {
    if (str.length() - index < length) return false;
    for (unsigned int i = 0; i < length; i++)
        if (str[index + i] != b[i])return false;
    return true;
}

template<unsigned int N>
inline bool cmppartstr_eq(const string& str, unsigned int index, const char (& b) [N] ) {
    return cmppartstr_eq(str, index, N-1, b);
}

std::string get_para(const string& str, unsigned int& iptr) {
    while (iptr < str.length() && str[iptr] == ' ')++iptr;
    if (iptr >= str.length())throw str_no_para;
    string ret;
    while (iptr < str.length() && str[iptr] != ' ' && str[iptr] != ','&& str[iptr]!='[')
    {
        ret.push_back(str[iptr]);
        ++iptr;
    }
    ++iptr;
    to_upper(ret);
    return ret;
}

unsigned get_imm(const std::string& id, tag_table& table, int el) {
    if (id.length() == 0)throw inv_id;
    if (('0' <= id[0] && id[0] <= '9')||id[0]=='-') return atoi(id.c_str());
    auto a = table.find(id);
    if (a != table.end())return a->second - (el << 2);
    throw inv_id;
}

unsigned int make_reg(const string& regid) {
    unsigned int ret=0;
    if (regid[0] == 'X') {
        ret = atoi(regid.c_str() + 1);
        if (ret > 31)throw inv_regid;
    }
    return ret;
}

void  jtype_imm(Instruction& ins, unsigned imm) {
    ins.jType.imm_10_1 = (imm & make_bits<11>()) >> 1;
    ins.jType.imm_11 = (imm >> 11) & 1;
    ins.jType.imm_19_12 = (imm >> 12) & make_bits<8>();
    ins.jType.imm_20 = (imm >> 20) & 1;
}

void btype_imm(Instruction& ins, unsigned imm) {
    ins.bType.imm_4_1 = (imm & make_bits<5>()) >> 1;
    ins.bType.imm_10_5 = (imm >> 5) & make_bits<6>();
    ins.bType.imm_11 = (imm >> 11) & 1;
    ins.bType.imm_12 = (imm >> 12) & 1;
}

void stype_imm(Instruction& ins, unsigned imm) {
    ins.sType.imm_4_0 = imm & make_bits<5>();
    ins.sType.imm_11_5 = (imm & make_bits<12>()) >> 5;
}

//zero for inv instruction
unsigned int make_1(const string& str, unsigned effective_line, tag_table& table) {
    Instruction insret;
    *reinterpret_cast<unsigned int*>(&insret) = 0;
    if (str.length() == 0)return 0;
    unsigned int iptr = 0;
    try {
        auto para = get_para(str, iptr);
        if (para.length() >= 1 && para[0] == '#')return 0;
        auto it = opcode_t.find(para);
        if (it != opcode_t.end())
            it->second(insret);
        if (insret.rType.opcode == 0) {
            if (str.back() == ':') {
                table.insert(make_pair(para.substr(0, str.length() - 1), effective_line*4));
                return 0;
            }
            else throw inv_instruction;
        }
        //GET PARAMS
        switch (insret.rType.opcode)
        {
        case OP_ALU://rty
            insret.rType.rs1 = make_reg( get_para(str, iptr));
            insret.rType.rs2 = make_reg( get_para(str, iptr));
            insret.rType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_ALU_IMM:
            if (insret.iType.funct3 == 0b101)insret.rType.funct7 |= 0b0100000;
        case OP_LOAD:
        case OP_JALR://ity
            insret.iType.rs1 = make_reg(get_para(str, iptr));
            insret.iType.imm |= get_imm(get_para(str, iptr), table, effective_line) & make_bits<12>();
            insret.iType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_LUI:
        case OP_AUIPC://uty
            insret.uType.imm_31_12 = get_imm(get_para(str, iptr), table, effective_line) & make_bits<20>();
            insret.uType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_JAL:
            jtype_imm(insret, get_imm(get_para(str, iptr),table, effective_line));
            insret.jType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_BTYPE:
            insret.bType.rs1 = make_reg(get_para(str, iptr));
            insret.bType.rs2 = make_reg(get_para(str, iptr));
            btype_imm(insret, get_imm(get_para(str, iptr), table, effective_line));
            break;
        case OP_STORE:
            insret. sType.rs1 = make_reg(get_para(str, iptr));
            stype_imm(insret, get_imm(get_para(str, iptr), table , effective_line));
            insret.sType.rs2 = make_reg(get_para(str, iptr));
            break;
        }
    }
    catch (int e) {
        if (e == str_no_para) {
            throw missing_para;
        }
        else throw e;
    }
    return *reinterpret_cast<unsigned int*>(&insret);
}

#define INPUT_SET 0x40000000
#define OUTPUT_SET 0x2000000

#define CMPEQ(_STR1, _STR2) (strcmp(_STR1, _STR2) == 0)

enum class ctrlflag {
    set_input,
    set_bout,
    set_nout
};

MY_API int fakemain(int argc, char** argv)
{
    int cf = 0 ,i =0;
    bool argv_alloct = false;
    int rl = 0, el = 0; unsigned int ins;
    char ibuf[8];
    tag_table table;
    std::string buf;
    if (argc == 0) {
        cout << "require input files\n";
        return 0;
    }
    buf = argv[0];
    if (buf.rfind("rv32casm") <buf.length() || buf.rfind("casm") < buf.length() )i++;
    cout << argv[0] << endl;
    ifstream input;
    ofstream out_bin, out_num;
    ctrlflag acf = ctrlflag::set_input;
    string nbout, nnout;
    //get args
    for (; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 || CMPEQ(argv[i], "-out") || CMPEQ(argv[i], "-bin_out"))
            acf = ctrlflag::set_bout;

        else if (strcmp(argv[i], "-ro") == 0 || CMPEQ(argv[i], "-readable_out") || CMPEQ(argv[i], "-num_out"))
            acf = ctrlflag::set_nout;

        else if (CMPEQ(argv[i], "-i") || CMPEQ(argv[i], "-input"))
            acf = ctrlflag::set_input;

        else if (CMPEQ(argv[i], "-l") || CMPEQ(argv[i], "-little_endian"))
            cf |= OUT_LITTLE_ENDIAN;

        else {
            switch (acf)
            {
            case ctrlflag::set_input:
                input.open(argv[i]);
                cf |= INPUT_SET;
                break;
            case ctrlflag::set_bout:
                out_bin.open(argv[i], ios::binary);
                cf |= OUTPUT_SET;
                cf |= OUT_BIN_EXEC;
                nbout = argv[i];
                break;
            case ctrlflag::set_nout:
                out_num.open(argv[i]);
                cf |= OUTPUT_SET;
                cf |= OUT_READBALE_FILE;
                nnout = argv[i];
                break;
            }
            acf = ctrlflag::set_input;
        }
    }
    //check args & fsio

    if (!(cf & INPUT_SET))
    {
        cout << "input file\n";
        cin >> buf;
        input.open(buf);
    }

    if (!(cf & OUTPUT_SET)) {
        cout << "output file\n";
        cin >> buf;
        out_num.open(buf);
        cf |= OUT_READBALE_FILE;
    }

    if (!(input.is_open() && (cf & OUT_BIN_EXEC ? out_bin.is_open() : true) && (cf & OUT_READBALE_FILE ?
        out_num.is_open() : true)) ){
        cout << "require input:\n";

            return 0;
    }
    
    //start
    try {
        while (getline(input, buf)) {
            ins = make_1(buf, el, table);
            if (ins == 0) {
                ++rl;
                continue;
            }
            ++el;
            ++rl;
            if (cf & OUT_READBALE_FILE)
                out_num <<hex << ins << endl;
            if (cf & OUT_BIN_EXEC)*reinterpret_cast<int*>(ibuf ) = cf& OUT_LITTLE_ENDIAN ? bl_endian_switch32(ins) : ins, 
                out_bin.write(ibuf, 4);
        }
    }
    catch (int e) {
        std::string errinfo;
        switch (e)
        {
        case inv_id:
            errinfo = "invalid identifier";
            break;
        case inv_regid:
            errinfo = "invalid register id";
            break;
        case inv_instruction:
        case missing_para:
            errinfo = "unknow instruction";
            break;
        }
        cout << "fetal: " << errinfo <<
            "\n@ line#" << el << ": " <<
            buf << '\n' << "#compile stopped#" << endl;
        if (cf & OUT_BIN_EXEC) remove(nbout.c_str());
        if (cf & OUT_READBALE_FILE) remove(nnout.c_str());
        return 0;
    }
    if (cf & OUT_BIN_EXEC) out_bin.close();
    if (cf & OUT_READBALE_FILE) out_num.close();
    return 0;
}

MY_API unsigned int make_1(const char* comm, int id_table_iptr)
{
    tag_table table;
    return make_1(string(comm), 0, id_table_iptr == -1 ?
        table : _internal_id_table[id_table_iptr].first);
}

MY_API int make_file(const char* src, unsigned cf, const char* dst_bin, const char* dst_read, FILE* out_log, const int id_table) {
    int rl = 0, el = 0; unsigned int ins;
    char ibuf[8];
    tag_table table;
    std::string buf;
    fstream input, out_bin, out_num;
    input.open(src);
    if (!input.is_open())return -1;
    if (cf & OUT_BIN_EXEC) {
        out_bin.open(dst_bin);
        if (out_bin.is_open()) { input.close(); return -1; }
    }
    if (cf & OUT_READBALE_FILE) out_num.open(dst_read);
    try {
        while (!input.eof()) {
            getline(input, buf);
            ins = make_1(buf, el, id_table == -1? table : _internal_id_table[id_table].first);
            if (ins == 0) {
                ++rl;
                continue;
            }
            ++el;
            ++rl;
            if (cf & OUT_READBALE_FILE)
                out_num << hex << ins << endl;
            if (cf & OUT_BIN_EXEC)*reinterpret_cast<int*>(ibuf) = cf & OUT_LITTLE_ENDIAN ? bl_endian_switch32(ins) : ins,
                out_bin.write(ibuf, 4);
        }
    }
    catch (int e) {
        std::string errinfo;
        switch (e)
        {
        case inv_id:
            errinfo = "invalid identifier";
            break;
        case inv_regid:
            errinfo = "invalid register id";
            break;
        case inv_instruction:
        case missing_para:
            errinfo = "unknow instruction";
            break;
        }
        if(out_log)fprintf(out_log, "fetal: %s\n@ line#%d: %s\n#compile stopped#\n",
            errinfo.c_str(), rl,buf.c_str());
        if (cf & OUT_BIN_EXEC) remove(dst_bin);
        if (cf & OUT_READBALE_FILE) remove(dst_read);
        return -1;
    }
    if (cf & OUT_BIN_EXEC) out_bin.close();
    if (cf & OUT_READBALE_FILE) out_num.close();
    return 0;
}

#define ADD_INSTRUCTION(_STRNAME_, _FUNCT_ ) opcode_t.insert(make_pair(string(_STRNAME_), &_FUNCT_))

void _init()
{
    opcode_t.insert(make_pair(string("LUI"), &op_make_lui));
    ADD_INSTRUCTION("AUIPC", op_make_auipc);
    ADD_INSTRUCTION("JAL", op_make_jal);
    ADD_INSTRUCTION("JALR", op_make_jalr);
    ADD_INSTRUCTION("BEQ", op_make_beq);
    ADD_INSTRUCTION("BNE", op_make_bne);
    ADD_INSTRUCTION("BLT", op_make_blt);
    ADD_INSTRUCTION("BGE", op_make_bge);
    ADD_INSTRUCTION("BLTU", op_make_bltu);
    ADD_INSTRUCTION("LB", op_make_lb);
    ADD_INSTRUCTION("LW", op_make_lw);
    ADD_INSTRUCTION("LH", op_make_lh);
    ADD_INSTRUCTION("LBU", op_make_lbu);
    ADD_INSTRUCTION("LHU", op_make_lhu);
    ADD_INSTRUCTION("SB", op_make_sb);
    ADD_INSTRUCTION("SH", op_make_sh);
    ADD_INSTRUCTION("SW", op_make_sw);
    ADD_INSTRUCTION("ADDI", op_make_addi);
    ADD_INSTRUCTION("SLTI", op_make_slti);
    ADD_INSTRUCTION("SLTIU", op_make_sltiu);
    ADD_INSTRUCTION("XORI", op_make_xori);
    ADD_INSTRUCTION("ORI", op_make_ori);
    ADD_INSTRUCTION("ANDI", op_make_andi);
    ADD_INSTRUCTION("SLLI", op_make_slli);
    ADD_INSTRUCTION("SRLI", op_make_srli);
    ADD_INSTRUCTION("SRAI", op_make_srai);
    ADD_INSTRUCTION("ADD", op_make_add);
    ADD_INSTRUCTION("SUB", op_make_sub);
    ADD_INSTRUCTION("SLL", op_make_sll);
    ADD_INSTRUCTION("SLT", op_make_slt);
    ADD_INSTRUCTION("SLTU", op_make_sltu);
    ADD_INSTRUCTION("XOR", op_make_xor);
    ADD_INSTRUCTION("SRL", op_make_srl);
    ADD_INSTRUCTION("SRA", op_make_sra);
    ADD_INSTRUCTION("OR", op_make_or);
    ADD_INSTRUCTION("AND", op_make_and);
}
