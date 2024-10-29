#pragma once
#ifndef S_Table_H_
#define S_Table_H_

#include<unordered_map>
#include<map>
#include<list>
#include<string>

#define ATTRIBUTE_ADDR_UNKNOWN 1

struct _var_attribute
{
	unsigned int is_const : 1;
	unsigned int is_unsigned : 1;
	unsigned int is_struct : 1;
	unsigned int is_array : 1;
	unsigned int ptr_cnt : 28;
};

class s_attribute
{
public:
	enum class Description {
		Null, Function, localVar, Structname, globalVar
	};
	Description s_class;//general_describe
	int Type;//ret-type for function, type for varable, id for var
	int local_addr_offset;
	_var_attribute attr;
	int size;
	std::pair<std::map<std::string, s_attribute>, std::list<s_attribute*> >* func_args;
	constexpr inline s_attribute() :s_class(Description::Null), Type(0), local_addr_offset(0), attr({ 0 }),size(0),func_args(nullptr) { }
	~s_attribute();
	s_attribute(const s_attribute&);
};

struct  sTableEntry {
	int frame_sp_base;
	std::map< std::string, s_attribute> map;
};

struct _type_description {
	int size;
	int id;//noneed to edit;
	bool is_struct;
	std::string name;
	struct structsubitems
	{
		_var_attribute attri;
		int offset;
		int tid;//typeid
	};
	std::map<std::string, std::pair<std::string, structsubitems > > _struct_descrip;
};

class Stable {
	std::unordered_map<std::string, s_attribute> global_defs;
	std::list<sTableEntry> localtables;
	std::map<std::string, _type_description*> _tytkbyname;
	std::map<int, _type_description*>_tytkbyid;
	std::map< std::string, s_attribute>* _func_var_p;
	int cnttypeida;
public:
	s_attribute* add_local(const std::string& name, const s_attribute& attribute);
	s_attribute* add_global(const std::string& name, const s_attribute& attribute);
	void pop();
	void push(int local_sp_addr_offset);
	const s_attribute* find(const std::string &name);
	Stable();
	~Stable();
	int add_ty(_type_description* d);
	void parse_func(const std::string& name);
	_type_description* find_type_by_id(int id);
	_type_description* find_type_by_name(const std::string& name);
};

#endif // !S_Table_H_
