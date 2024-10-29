#pragma once
#include"s_table.h"
#include"tks.h"
#include<sstream>

class parsing_o_s {
protected:
	class mBuffer {
		public:
		std::string ixname;
		unsigned pc;
		int id;
		std::string buf;
	};
	FILE* FOUT;
	std::list<mBuffer* > vcur;
	std::list<mBuffer* > vbuf;
	mBuffer* cur;
	unsigned totalpc;
	unsigned brace_uid_gen;
	void write_buffer();
public:
	unsigned int GetPC();
	void push(const char* name);
	void pop();
	int getid();
	void add_instruction(const std::string& ins);
	int get_new_id();
	void make_buf();
	void pop_buf();
	parsing_o_s(FILE* outbuf);
	void add_instruction_back(const std::string& ins);
	std::string& getbufref();
};

struct Machine_state {
	int sp_offset;
	int frame_ptr_offset;
};

int parse_all(Stable& table, Token* tk, parsing_o_s& out);

Token* parse_enum(Stable& table, Token* tk, parsing_o_s& out);

Token* parse_struct(Stable& table, Token* tk, parsing_o_s& out);

Token* parse_func_def(Stable& table, Token* tk,Machine_state& ms, parsing_o_s& out);

Token* parse_var_decl(Stable& table, Token* tk, Machine_state& ms, parsing_o_s& out);//local var only

struct LoopCtrlinfo {
	unsigned loop_id;
	unsigned loop_ctrl_start;
	unsigned loop_ctrl_end;
	int loop_sp;
};

Token* parse_brace_(Stable& table, Token*, Machine_state& ms, parsing_o_s& out, const char* reg_name, LoopCtrlinfo* loopctrl, int cnt_rec_depth);

//stop when meet ';' or ')'
Token* parse_statment(Stable& table, Token*, Machine_state& ms, parsing_o_s& out);

Token* parse_var_global(Stable& table, Token* tk, parsing_o_s& out);

Token* cedclcall(Machine_state& ms, parsing_o_s& o, const char* fn, Token*);

//code gen

//push s0, s0 = sp + s

#define RVCCCALL_STACK_OFFSET 64
void push_fp(Machine_state & ms, parsing_o_s & o);

void rvcccall(Machine_state& ms, parsing_o_s& o);

void rvccret(Machine_state& ms, parsing_o_s& o);

#define FASTRVCC_INSTRUCTION_SIZE 12

void rvcccall_fast(Machine_state& ms, parsing_o_s& o);//store ra and a7

void rvccret_fast(Machine_state& ms, parsing_o_s& o);

void cedclretcu(Machine_state& ms, parsing_o_s& o, const char* fn);

void _op_assignlval(s_attribute*, parsing_o_s& o, const std::string& rval);