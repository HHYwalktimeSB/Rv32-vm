// rv32casm.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include"../Rv32-vm/_cpu.h"
#include<string>
#include<fstream>
#include<vector>
#include<iomanip>

using tag_table = std::vector< std::pair<std::string, unsigned int> >;

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
    while (iptr < str.length() && str[iptr] != ' ' && str[iptr] != ',');
    {
        ret.push_back(str[iptr]);
        ++iptr;
    }
    ++iptr;
    return ret;
}

unsigned get_imm(const std::string& id, tag_table& table) {
    if (id.length() == 0)throw inv_id;
    if ('0' <= id[0] && id[0] <= '9') return atoi(id.c_str());
    for (auto& a : table) {
        if (id == a.first) return a.second*4;
    }
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

unsigned int make_1(const string& str, unsigned effective_line, tag_table& table) {
    bool get_opcode = false;
    Instruction insret;
    *reinterpret_cast<unsigned int*>(&insret) = 0;
    try {
        unsigned int iptr = 0;
        auto para = get_para(str, iptr);
        to_upper(para);
        if (para.length() < 2)throw inv_instruction;
        switch (para[0])
        {
        case '#':
            return 0;
        case 'A':
            if (cmppartstr_eq(para, 1, "DD")) {
                if (para.length() == 3) insret.rType.opcode = OP_ALU;
                else if (para[3] == 'I')insret.iType.opcode = OP_ALU_IMM;
                insret.iType.funct3 = 0b000;
            }
            else if (cmppartstr_eq(para, 1, "ND")) {
                if (para.length() == 3) insret.rType.opcode = OP_ALU;
                else if (para[3] == 'I')insret.iType.opcode = OP_ALU_IMM;
                insret.iType.funct3 = 0b111;
            }
            else if (cmppartstr_eq(para, 1, "UIPC")) 
                insret.uType.opcode = OP_AUIPC;
            break;
        case 'B':
            if (cmppartstr_eq(para, 1, "EQ")) {
                insret.bType.opcode = OP_BTYPE;
                insret.bType.funct3 = 0b000;
            }
            else if (cmppartstr_eq(para, 1, "NE")) {
                insret.bType.opcode = OP_BTYPE;
                insret.bType.funct3 = 0b001;
            }
            else if (cmppartstr_eq(para, 1, "LT")) {
                if (para.length() == 3) insret.bType.opcode = OP_BTYPE, insret.bType.funct3 = 0b100;
                else if (para[3] == 'U')insret.bType.opcode = OP_BTYPE, insret.bType.funct3 = 0b110;
            }
            else if (cmppartstr_eq(para, 1, "GE")) {
                if (para.length() == 3) insret.bType.opcode = OP_BTYPE, insret.bType.funct3 = 0b101;
                else if (para[3] == 'U')insret.bType.opcode = OP_BTYPE, insret.bType.funct3 = 0b111;
            }
            break;
        case 'E':
            if (cmppartstr_eq(para, 1, "CALL")) {
                throw inv_instruction;
            }
            else if(cmppartstr_eq(para, 1, "BREAK")) {
                throw inv_instruction;
            }
            break;
        case 'J':
            if (cmppartstr_eq(para, 1, "AL")) {
                if (para.length() == 3)insret.jType.opcode = OP_JAL;
                else if (para[3] == 'R') insret.iType.opcode = OP_JALR, insret.iType.funct3 = 0;
            }
            break;
        case 'L':
            if (cmppartstr_eq(para, 1, "UI")) {
                insret.uType.opcode = OP_LUI;
            }
            else if (para.length() <= 3) {
                switch (para[1])
                {
                case 'B':
                    insret.iType.opcode = OP_LOAD;
                    if (para.length() == 2)insret.iType.funct3 = 0;
                    else insret.iType.funct3 = 0b100;
                    break;
                case 'H':
                    insret.iType.opcode = OP_LOAD;
                    if (para.length() == 2)insret.iType.funct3 = 1;
                    else insret.iType.funct3 = 0b101;
                    break;
                case 'W':
                    insret.iType.opcode = OP_LOAD;
                    insret.iType.funct3 = 2;
                    break;
                }
            }
            break;
        case 'O':
            if (para.length()==3&& cmppartstr_eq(para, 1, "RI")) {
                insret.iType.opcode = OP_ALU_IMM;
                insret.iType.funct3 = 110;
            }
            else if (para.length() == 2 && para[1] == 'R') {
                insret.iType.opcode = OP_ALU;
                insret.iType.funct3 = 110;
            }
            break;
        case 'S':
            if (cmppartstr_eq(para, 1, "LL")) {
                if (para.length() == 3) 
                    insret.rType.opcode = OP_ALU,
                    insret.rType.funct3 = 0b001;
                else if (para[3] == 'I') 
                    insret.rType.opcode = OP_ALU_IMM,
                    insret.rType.funct3 = 0b001;
            }
            else if (cmppartstr_eq(para, 1, "RL")|| cmppartstr_eq(para, 1, "RA"))
            {
                if (para.length() == 3)
                    insret.rType.opcode = OP_ALU,
                    insret.rType.funct3 = 0b101;
                else if(para[3] == 'I') 
                    insret.rType.opcode = OP_ALU_IMM,
                    insret.rType.funct3 = 0b101;
            }
            else if (cmppartstr_eq(para,1,"LT")) {
                if (para.length() == 3)
                    insret.rType.opcode = OP_ALU,
                    insret.rType.funct3 = 0b010;
                else if (para[3] == 'U')
                    insret.rType.opcode = OP_ALU,
                    insret.rType.funct3 = 0b011;
            }
            else if (cmppartstr_eq(para, 1, "UB")) {

            }
            else if (para.length() == 2) {
                switch (para[1])
                {
                case 'B':
                    insret.sType.opcode = OP_STORE;
                    insret.sType.funct3 = 0;
                    break;
                case 'H':
                    insret.sType.opcode = OP_STORE;
                    insret.sType.funct3 = 1;
                    break;
                case 'W':
                    insret.sType.opcode = OP_STORE;
                    insret.sType.funct3 = 2;
                    break;
                }
            }
            break;
        case 'X':
            if (cmppartstr_eq(para, 1, "OR")) {
                if (para.length() == 3) insret.rType.opcode = OP_ALU, insret.rType.funct3 = 0b100;
                else insret.iType.opcode = OP_ALU_IMM, insret.rType.funct3 = 0b100;
            }
            break;
        }
        //switch_end
        if (insret.rType.opcode == 0) {
            if (str.back() == ':') {
                table.push_back(make_pair(str.substr(0, str.length() - 1), effective_line));
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
        case OP_LOAD:
        case OP_JALR://ity
            insret.iType.rs1 = make_reg(get_para(str, iptr));
            insret.iType.imm = get_imm(get_para(str, iptr), table) & make_bits<12>();
            insret.iType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_LUI:
        case OP_AUIPC://uty
            insret.uType.imm_31_12 = get_imm(get_para(str, iptr), table) & make_bits<20>();
            insret.uType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_JAL:
            jtype_imm(insret, get_imm(get_para(str, iptr),table));
            insret.jType.rd = make_reg(get_para(str, iptr));
            break;
        case OP_BTYPE:
            insret.bType.rs1 = make_reg(get_para(str, iptr));
            insret.bType.rs2 = make_reg(get_para(str, iptr));
            btype_imm(insret, get_imm(get_para(str, iptr), table));
            break;
        case OP_STORE:
            insret. sType.rs1 = make_reg(get_para(str, iptr));
            stype_imm(insret, get_imm(get_para(str, iptr), table));
            insret.sType.rs2 = make_reg(get_para(str, iptr));
            break;
        }
    }
    catch (int e) {
        if (e == str_no_para) {
            if (get_opcode)throw missing_para;
            return 0;
        }
        else
            throw e;
    }
    return *reinterpret_cast<unsigned int*>(&insret);
}

#define OUT_BIN_EXEC 1
#define OUT_LITTLE_ENDIAN 2
#define OUT_READBALE_FILE 4
#define INPUT_SET 0x40000000
#define OUTPUT_SET 0x2000000

#define CMPEQ(_STR1, _STR2) (strcmp(_STR1, _STR2) == 0)

enum class ctrlflag {
    set_input,
    set_bout,
    set_nout
};

int main(int argc, char** argv)
{
    int cf = 0 ,i =0;
    if (argc == 0) {
        cout << "require input files\n";
        return 0;
    }
    if (cmppartstr_eq(string(argv[0]), 0, "rv32csam") || cmppartstr_eq(string(argv[0]), 0, "csam"))i++;
    fstream input, out_bin, out_num;
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
                input.open(argv[i], ios::in);
                cf |= INPUT_SET;
                break;
            case ctrlflag::set_bout:
                out_bin.open(argv[i], ios::out | ios::binary);
                cf |= OUTPUT_SET;
                cf |= OUT_BIN_EXEC;
                nbout = argv[i];
                break;
            case ctrlflag::set_nout:
                out_bin.open(argv[i], ios::out);
                cf |= OUTPUT_SET;
                cf |= OUT_READBALE_FILE;
                nnout = argv[i];
                break;
            }
            acf = ctrlflag::set_input;
        }
    }
    //check args & fsio

    if (!((cf & INPUT_SET) && (cf & OUTPUT_SET)))
    {
        cout << "fatel: input or output file missing\n";
        return 0;
    }
    if (!(input.is_open() && (cf & OUT_BIN_EXEC ? out_bin.is_open() : true) &&
        (cf & OUT_READBALE_FILE ? out_num.is_open() : true))) {
        cout << "fatel: failed to open some file(s)\n";
        return 0;
    }
    //start

    int rl = 0, el = 0; unsigned int ins;
    char ibuf[8];
    tag_table table;
    std::string buf;
    try {
        while (!input.eof()) {
            getline(input, buf);
            ins = make_1(buf, el, table);
            if (ins != 0)++rl;
            ++el;
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
        cout << "fatel: " << errinfo <<
            "\n@ line#" << el << ": " <<
            buf << '\n' << "#compile stopped#" << endl;
        if (cf & OUT_BIN_EXEC) remove(nbout.c_str());
        if (cf & OUT_READBALE_FILE) remove(nnout.c_str());
    }
    if (cf & OUT_BIN_EXEC) out_bin.close();
    if (cf & OUT_READBALE_FILE) out_num.close();
    return 0;
}