#include "pch.h"
#include "parsing.h"

using namespace std;

bool is_token_a_typename(Token* tk, Stable& table){
	return (tk->type == TK_TYPE_KEYWORD && (tk->val == KW_TYPENAME_CHAR || tk->val == KW_TYPENAME_INT
		|| tk->val == KW_TYPENAME_SHORT));
}

bool is_token_a_structname(Token* tk, Stable& tablw) {
	return false;
}

//1for func 0 for var
int is_func_or_var(Token* tk, Stable& table) {
	while (tk->str.str[0] != ';') {
		if (is_token_a_typename(tk, table)) {
			tk = tk->next;
			continue;
		}
		if (tk->type == TK_TYPE_ID) break;
		tk = tk->next;
	}
	tk = tk->next;
	if (tk->type == TK_TYPE_STOP && tk->str.str[0] == '(')return 1;//func
	else return 0;
}

int parse_all(Stable& table, Token* tk, parsing_o_s& out)
{
	Machine_state ms;
	while (tk) {
		if (tk->type == TK_TYPE_KEYWORD) {
			switch (tk->val)
			{
			case KW_TYPENAME_CHAR:
			case KW_TYPENAME_INT:
			case KW_TYPENAME_D_CONST:
			case KW_TYPENAME_D_UNSIGNED:
			case KW_TYPENAME_SHORT:
				if (is_func_or_var(tk, table)) {
					tk = parse_func_def(table, tk, ms, out);
				}
				else {
					tk = parse_var_global(table, tk, out);
				}
				break;
			case KW_STRUCT:
				if (is_token_a_structname(tk = tk->next,table)) {
					if (is_func_or_var(tk, table)) 
						tk = parse_func_def(table, tk, ms, out);
					else
						tk = parse_var_global(table, tk, out);
				}
				tk = parse_struct(table, tk, out);
				break;
			case KW_ENUM:
				tk = parse_enum(table, tk, out);
				break;
			default:
				throw "syntax error";
			}
		}
		else if (tk->type == TK_TYPE_ID) {
			if (is_token_a_typename(tk, table)) {
				if (is_func_or_var(tk, table)) {
					tk = parse_func_def(table, tk, ms, out);
				}
				else {
					tk = parse_var_global(table, tk, out);
				}
			}
		}
		else throw "syntax error";
		if(tk)tk = tk->next;
	}
	return 0;
}

Token* parse_enum(Stable& table, Token* tk, parsing_o_s& out)
{
	throw "enum not supported";
}

Token* parse_struct(Stable& table, Token* tk, parsing_o_s& out)
{
	throw "struct not supported";
}

Token* parse_var_single(Stable& table, s_attribute* x, Token* tk, std::string& name) {
	*reinterpret_cast<unsigned int*> (&(x->attr)) = 0;
	_type_description* tmp = NULL;
	while (tk) {
		if (tk->type = TK_TYPE_KEYWORD) {
			switch (tk->val)
			{
			case KW_TYPENAME_INT:
				tmp = table.find_type_by_name("int");
				x->Type = tmp->id;
				x->size = tmp->size;
				break;
			case KW_TYPENAME_CHAR:
				tmp = table.find_type_by_name("char");
				x->Type = tmp->id;
				x->size = tmp->size;
				break;
			case KW_TYPENAME_SHORT:
				tmp = table.find_type_by_name("short");
				x->Type = tmp->id;
				x->size = tmp->size;
				break;
			case KW_TYPENAME_D_CONST:
				x->attr.is_const = 1;
				break;
			case KW_TYPENAME_D_UNSIGNED:
				x->attr.is_unsigned = 1;
				break;
			case KW_STRUCT:
				x->attr.is_struct = 1;
				tk = tk->next;
				tmp = table.find_type_by_name(std::string(tk->str.str, tk->str.length));
				x->Type = tmp->id;
				x->size = tmp->size;
			}
		}
		else {
			if (tk->str.str[0] == ';' || tk->str.str[0] == '=' || tk->str.str[0] == ',' || tk->str.str[0] == ')')return tk;
			else if (tk->str.str[0] == '*')x->attr.ptr_cnt += 1;
			else {
				name = tk->str.str;
				tk = tk->next;
				break;
			}
		}
		tk = tk->next;
	}
	if (tk!=nullptr&& tk->str.str[0] == '[') {
		x->attr.is_array = 1;
		tk = tk->next;
		if (tk->type == TK_TYPE_NUMBER) {
			x->size *= tk->val;
		}
		else throw "array decl a[ ... ] ... not a const";
		tk = tk->next;
		if (tk->str.str[0] != ']')throw "err array decl a[ ... ] , ] is missing";
		tk = tk->next;
	}
	return tk;
}

Token* parse_func_def(Stable& table, Token* tk, Machine_state& ms, parsing_o_s& out)
{
	s_attribute sa;
	sa.s_class = s_attribute::Description::Function;
	std::string name;
	parse_var_single(table, &sa, tk, name);
	if (tk->str.str[0] != '(')throw "error";
	tk = tk->next;

	//parse arguments
	sa.func_args = new std::pair<std::map<std::string, s_attribute>, std::list<s_attribute*> >();
	while (tk) {
		if (tk->str.str[0] == ')')break;
		std::string vname;
		s_attribute s;
		tk = parse_var_single(table, &s, tk, vname);
		sa.func_args->second.push_back ( &(sa.func_args->first.insert(std::make_pair(vname, s)).first->second) );
	}
	//va allocate

	int fpoffset = 8;
	int align;
	for (auto a : sa.func_args->second) {
		align = a->size % 4;
		if (align) {
			align = 4 - align;
			fpoffset += align;
		}
		a->local_addr_offset = fpoffset;
		fpoffset += a->size;
	}

	table.add_global(name, sa);
	tk = tk->next;
	if (tk->str.str[0] == ';')throw "error!not support function declartion only";

	tk = parse_brace_(table, tk, ms, out, name.c_str(), nullptr, 0);

	return tk;
}

Token* parse_var_decl(Stable& table, Token* tk, Machine_state& ms, parsing_o_s& out)//local var
{
	std::string name;
	s_attribute s;
	int alignment = 0;
	memset(&s, 0, sizeof(s));
	tk = parse_var_single(table,&s, tk, name);
	if (s.size == 0 || s.Type == 0) throw " error vardecl... missing";
	s.s_class = s_attribute::Description::localVar;
	//align sp
	 switch (s.Type)
	{
	case 1:
		break;
	case 2:
		if (ms.sp_offset % 2 != 0) {
			ms.sp_offset -= 1; 
			out.add_instruction("addi sp, -1, sp");
		}
		break;
	case 3:
	default:
		alignment = -ms.sp_offset % 4;
		alignment = alignment % 4;
		if (alignment == 0)break;
		alignment = 4 - alignment;
		out.add_instruction(std::string("addi sp, -") + std::to_string(alignment) + ", sp");
		ms.sp_offset -= alignment;
		break;
	}
	ms.sp_offset -= s.size;
	out.add_instruction(std::string("addi sp, -") + std::to_string(s.size) + ", sp");
	s.local_addr_offset = ms.sp_offset - ms.frame_ptr_offset;
	if (table.add_local(name, s) == nullptr) throw "redef";
	if (tk->str.str[0] == '=') {
		tk = parse_statment(table, tk->next, ms, out); 
		_op_assignlval(&s, out, "a0");
	}
	else tk = tk->next;

	return tk;
}

static inline void mkw_ctrl_continue(Machine_state& ms, parsing_o_s & out, const char* name, LoopCtrlinfo* loop_ctrl) {
	out.add_instruction(std::string("addi sp, ") + std::to_string(loop_ctrl->loop_sp - ms.sp_offset) + ", sp");
	out.add_instruction(std::string("jal x0, @_br_") + name + std::to_string(loop_ctrl->loop_id) + ":-addr " +
		std::to_string(loop_ctrl->loop_ctrl_start));
}
static inline void mkw_ctrl_break(Machine_state& ms, parsing_o_s& out, const char* name, LoopCtrlinfo* loop_ctrl) {
	out.add_instruction(std::string("addi sp, ") + std::to_string(loop_ctrl->loop_sp - ms.sp_offset) + ", sp");
	out.add_instruction(std::string("jal x0, @_br_") + name + std::to_string(loop_ctrl->loop_id) + ":-addr" +
		std::to_string(loop_ctrl->loop_ctrl_end));
}

static inline Token* mkw_ctrl_return(Stable& table, Machine_state& ms, Token* tk, parsing_o_s& out) {
	tk = parse_statment(table, tk->next, ms, out);
	out.add_instruction("add x0 fp sp");
	out.add_instruction("lw sp[0], ra");
	out.add_instruction("lw sp[4], fp");
	out.add_instruction("addi sp, 8 sp");
	out.add_instruction("jalr x0, ra, 0");
	return tk;
}

static inline Token* mkw_ctrl_goto(Stable& table, Machine_state& ms, Token* tk, parsing_o_s& out) {
	tk = tk->next;
	out.add_instruction(std::string("$addi a7 @-calc sub -v") + tk->str.str + ".fpval ");
	out.add_instruction(std::string("$JMP @-v ") + tk->str.str + ".vaddr");
	tk = tk->next;
	return tk;
}

static inline Token* _4keyword_switch(Stable& table, Machine_state& ms, Token* tk, parsing_o_s& out, LoopCtrlinfo* loop_ctrl, const char* name) {
	switch (tk->val)
	{
	case KW_CTRL_CONTINUE:
		mkw_ctrl_continue(ms, out, name, loop_ctrl);
		tk = tk->next->next;
		break;
	case KW_CTRL_BREAK:
		mkw_ctrl_break(ms, out, name, loop_ctrl);
		tk = tk->next->next;
		break;
	case KW_CTRL_GOTO:
		tk = mkw_ctrl_goto(table, ms, tk, out);
		break;
	case KW_CTRL_RETURN:
		tk = mkw_ctrl_return(table, ms, tk, out);
		break;
	}
	return tk;
}

Token* parse_brace_(Stable& table, Token* tk, Machine_state& ms, parsing_o_s& out, const char* name, LoopCtrlinfo* loop_ctrl, int cnt_rec_depth)
{
	out.push(name);
	tk = tk->next;
	int wdmnd = 0;
	if (cnt_rec_depth == 0) {
		ms.sp_offset -= 8;
		wdmnd = ms.frame_ptr_offset = ms.sp_offset;
		out.add_instruction("$addi sp, -8, sp");
		out.add_instruction("$sw sp[0], ra");
		out.add_instruction("$sw sp[4], fp");
		out.add_instruction("add sp, x0, fp");
	}
	else {
		wdmnd = ms.sp_offset;
		out.add_instruction("add sp, x0, a7");
	}
	table.push(ms.frame_ptr_offset);
	while (tk)
	{
		if (tk->type == TK_TYPE_KEYWORD) {
			switch (tk->val)
			{
			case KW_STRUCT:
			case KW_TYPENAME_INT:
			case KW_TYPENAME_CHAR:
			case KW_TYPENAME_SHORT:
			case KW_TYPENAME_D_UNSIGNED:
			case KW_TYPENAME_D_CONST:
				tk = parse_var_decl(table, tk, ms, out);
				break;
			case KW_CTRL_IF:
				tk = tk->next->next;
				tk = parse_statment(table, tk, ms, out);
				if (tk->str.str[0] == '{') {
					out.add_instruction(std::string("beq a0, x0, ") + std::to_string(out.GetPC() + (12 + 2 * FASTRVCC_INSTRUCTION_SIZE)));
					rvcccall_fast(ms, out);
					out.add_instruction(std::string("jal ra, @_br_") + name + std::to_string(out.get_new_id()));
					rvccret_fast(ms, out);
					out.add_instruction("addi x0, 1,a0");
					tk = parse_brace_(table, tk, ms, out, name, loop_ctrl, cnt_rec_depth + 1);
				}
				else if (tk->type == TK_TYPE_KEYWORD)
					tk = _4keyword_switch(table, ms, tk, out, loop_ctrl, name);
				else{
					out.make_buf();
					if (tk->type == TK_TYPE_KEYWORD)tk = _4keyword_switch(table, ms, tk, out, loop_ctrl, name);
					else tk = parse_statment(table, tk, ms, out);
					out.add_instruction_back("beq a0, x0, " + std::to_string(out.GetPC() + 8));
					out.pop_buf();
					out.add_instruction("addi x0, 1,a0");
				}
				break;
			case KW_CTRL_ELSE:
			{
				tk = tk->next;
				auto start_pc = out.GetPC();
				auto modix = out.getbufref().length() + 12;
				Token* brace_ptr = nullptr;
				out.add_instruction("bne a0, x0,                                   ");
				if (tk->str.str[0] == '{') {
					rvcccall_fast(ms, out);
					out.add_instruction(std::string("jal ra, @_br_") + name + std::to_string(out.get_new_id()));
					rvccret_fast(ms, out);
					brace_ptr = tk;
				}
				else if (tk->type == TK_TYPE_KEYWORD) {
					if (tk->val == KW_CTRL_IF) {
						tk = tk->next->next;
						tk = parse_statment(table, tk, ms, out);
						if (tk->str.str[0] == '{') {
							out.add_instruction(std::string("beq a0, x0, ") + std::to_string(out.GetPC() + (12 + 2 * FASTRVCC_INSTRUCTION_SIZE)));
							rvcccall_fast(ms, out);
							out.add_instruction(std::string("jal ra, @_br_") + name + std::to_string(out.get_new_id()));
							rvccret_fast(ms, out);
							out.add_instruction("addi x0, 1,a0");
							brace_ptr = tk;
						}
						else {
							out.make_buf();
							if (tk->type == TK_TYPE_KEYWORD)tk = _4keyword_switch(table, ms, tk, out, loop_ctrl, name);
							else tk = parse_statment(table, tk, ms, out);
							out.add_instruction_back("beq a0, x0, " + std::to_string(out.GetPC() + 8));
							out.pop_buf();
							out.add_instruction("addi x0, 1,a0");
						}
					}
					else tk = _4keyword_switch(table, ms, tk, out, loop_ctrl, name);
				}
				else tk = parse_statment(table, tk, ms, out);
				auto endpc = out.GetPC();
				start_pc = endpc - start_pc;
				_itoa_s(start_pc, const_cast<char*>(out.getbufref().data()) + modix,32,  10);
				if(brace_ptr) tk = parse_brace_(table, brace_ptr, ms, out, name, loop_ctrl, cnt_rec_depth + 1);
				break;
			}
			case KW_CTRL_WHILE:
			{
				tk = tk->next->next;
				LoopCtrlinfo* ctrl = new LoopCtrlinfo;
				Token* brace_ptr = nullptr;
				ctrl->loop_id = out.getid();
				if (tk->str.str[0] == '{') rvcccall_fast(ms, out);
				ctrl->loop_ctrl_start = out.GetPC();
				tk = parse_statment(table, tk, ms, out);
				if (tk->str.str[0] == '{') {
					brace_ptr = tk;
					out.add_instruction(std::string("beq a0, x0, ") + std::to_string(out.GetPC() + 12));
					out.add_instruction(std::string("jal ra, @_br_") + name + std::to_string(out.get_new_id()));
				}
				else {
					out.make_buf();
					tk = parse_statment(table, tk, ms, out);
					out.add_instruction_back("beq a0, x0, " + std::to_string(out.GetPC() + 8));
					out.pop_buf();
				}
				out.add_instruction(std::string("jal x0, ") + std::to_string((int)ctrl->loop_ctrl_start - (int)out.GetPC()));
				if (brace_ptr) { 
					ctrl->loop_sp = ms.sp_offset;
					ctrl->loop_ctrl_end = out.GetPC();
					tk = parse_brace_(table, tk, ms, out, name, ctrl, cnt_rec_depth + 1);
					rvccret_fast(ms, out); 
				}
				delete ctrl;
				ctrl = nullptr;
				break;
			}
			case KW_CTRL_DO:
			{
				tk = tk->next;
				Token* brace_stat = nullptr;
				LoopCtrlinfo* ctrl = new LoopCtrlinfo;
				int cntleft = 1;
				ctrl->loop_id = out.getid();
				if (tk->str.str[0] == '{')rvcccall_fast(ms, out);
				ctrl->loop_ctrl_start = out.GetPC();
				if (tk->str.str[0] == '{') { brace_stat = tk; tk = tk->next;
					out.add_instruction(std::string("jal ra, @_br_") + name + std::to_string(out.get_new_id()));
					while (cntleft > 0) { if (tk->str.str[0] == '}')--cntleft; else if (tk->str.str[0] == '{')++cntleft; tk = tk->next;  }
				}
				else {
					tk = parse_statment(table, tk, ms, out);
				}
				if ((TK_TYPE_KEYWORD == tk->type) && (tk->val = KW_CTRL_WHILE)) {
					tk = tk->next->next;
					tk = parse_statment(table, tk, ms, out);
					out.add_instruction(std::string("bne a0, x0, ") + std::to_string((int)ctrl->loop_ctrl_start - (int)out.GetPC()));
					tk = tk->next;
				}
				else throw "syntax do...while, while is missing";
				if (brace_stat) { 
					ctrl->loop_ctrl_end = out.GetPC(); ctrl->loop_sp = ms.sp_offset;
					tk = parse_brace_(table, brace_stat, ms, out, name, ctrl, cnt_rec_depth + 1);
					rvccret_fast(ms, out); 
				}
				delete ctrl;
				ctrl = nullptr;
				break;
			}
			case KW_CTRL_FOR:
				throw "for loop currently not supported";
				break;
			case KW_CTRL_CONTINUE:
				mkw_ctrl_continue(ms, out, name, loop_ctrl);
				tk = tk->next;
				break;
			case KW_CTRL_BREAK:
				mkw_ctrl_break(ms, out, name, loop_ctrl);
				tk = tk->next;
				break;
			case KW_CTRL_GOTO:
				tk = mkw_ctrl_goto(table, ms, tk, out);
				break;
			case KW_CTRL_RETURN:
				tk = mkw_ctrl_return(table, ms, tk, out);
				break;
			}
		}
else {
	if (tk->str.str[0] == '}')break;
	if (tk->str.str[0] == ';')tk = tk->next;
	else tk = parse_statment(table, tk, ms, out);
		}
	}
	if (cnt_rec_depth == 0) {
		ms.sp_offset = ms.frame_ptr_offset + 8;
		out.add_instruction("add x0 fp sp");
		out.add_instruction("lw sp[0], ra");
		out.add_instruction("lw sp[4], fp");
		out.add_instruction("addi sp, 8 sp");
	}
	else {
		ms.sp_offset = wdmnd;
		out.add_instruction("add a7, x0, sp");
	}
	out.add_instruction("jalr x0, ra, 0");
	table.pop();
	out.pop();
	return tk ? tk->next : nullptr;
}

class _NodeUnionTy {
public:
	Token* tkref;
	struct 
	{
		unsigned is_lval : 1;
		unsigned reserved : 31;
	}attr;

};

Token* parse_statment(Stable& table, Token* tk, Machine_state& ms, parsing_o_s& out)
{
	//sepcial case
	if (tk->next!=nullptr && tk->next->str.str[0] == ':') {
		out.add_instruction(std::string("@-cvar ") + tk->str.str + " -part vaddr:" + std::to_string(out.GetPC()) +
		"-part sp:" + std::to_string(ms.sp_offset) );
		return tk->next->next;
	}
	//normal case
	stack<_exprNode> data;
	stack<Token*> op;
	int left_brace_cnt = 0;
	_exprNode tmp;
	while (tk) {
		if (tk->str.str[0] == ';' || tk->str.str[0] == ',')break;
		if (tk->str.str[0] == ')' || left_brace_cnt == 0)break;
		if (tk->str.str[0] == '(' && tk->next->type == TK_TYPE_KEYWORD) {
			tk = tk->next;
			//cast 
			while (tk) {

			}
		}
		if (tk->type == TK_TYPE_ID || tk->type == TK_TYPE_NUMBER) {
			//nodes.push(tk);
		}
		else {
			op.push(tk);
		}
	}
}

Token* parse_var_global(Stable& table, Token* tk, parsing_o_s& out)
{
	throw "error global var todo!";
	return nullptr;
}

void rvcccall(Machine_state& ms, parsing_o_s& o) {
	//push ra, push a1-a7 
	ms.sp_offset -= 64;
}

void rvccret(Machine_state& ms, parsing_o_s& o)
{
	//pop a7-a1, pop ra
	ms.sp_offset += 64;
}

void rvcccall_fast(Machine_state& ms, parsing_o_s& o)
{
	o.add_instruction("$addi sp, -8, sp");
	o.add_instruction("$sw sp[0], ra");
	o.add_instruction("$sw sp[4], a7");
	ms.sp_offset -= 8;
}

void rvccret_fast(Machine_state& ms, parsing_o_s& o)
{
	o.add_instruction("$lw sp[0], ra");
	o.add_instruction("$lw sp[4], a7");
	o.add_instruction("$addi sp, 8, sp");
	ms.sp_offset -= 8;
}

void _op_assignlval(s_attribute*, parsing_o_s& o, const std::string& rval)
{
	throw "err op_assign_val todo!";
}

int parsing_o_s::get_new_id()
{
	return this->brace_uid_gen;
}

void parsing_o_s::make_buf()
{
	cur = new mBuffer;
	cur->id = vcur.back()->id;
	cur->pc = 0;
}

void parsing_o_s::pop_buf()
{
	vcur.back()->buf.append(cur->buf);
	vcur.back()->pc += cur->pc;
	delete cur;
	cur = vcur.back();
}

parsing_o_s::parsing_o_s(FILE* outbuf)
{
	FOUT = outbuf;
	cur = nullptr;
	brace_uid_gen = 0;
	totalpc = 0;
}

void parsing_o_s::add_instruction_back(const std::string& ins)
{
	vcur.back()->buf.append(ins);
	vcur.back()->buf.push_back('\n');
	vcur.back()->pc += 4;
}

std::string& parsing_o_s::getbufref()
{
	return cur->buf;
}

void parsing_o_s::write_buffer()
{
	for (auto a : vbuf) {
		fprintf(FOUT, "@-t _br_%s%d:\n", a->ixname.c_str(), a->id);
		fwrite(a->buf.c_str(), 1, a->buf.length(), FOUT);
		totalpc += a->pc;
		delete a;
	}
	vbuf.clear();
}

unsigned int parsing_o_s::GetPC()
{
	return cur->id;
}

void parsing_o_s::push(const char* name)
{
	mBuffer *buf = new mBuffer;
	buf->id = brace_uid_gen;
	buf->pc = 0;
	buf->ixname = name;
	++brace_uid_gen;
	vcur.push_back(buf);
	cur = vcur.back();
}

void parsing_o_s::pop()
{
	if (vcur.empty())throw "internal err";
	vbuf.push_back(cur);
	vcur.pop_back();
	if (vcur.empty()) {
		this->write_buffer();
		vbuf.clear();
		cur = nullptr;
	}
	else {
		cur = vbuf.back();
	}
}

int parsing_o_s::getid()
{
	return cur->id;
}

void parsing_o_s::add_instruction(const std::string& ins)
{
	cur->buf.append(ins);
	cur->buf.push_back('\n');
	cur->pc += 4;
}
