#include "pch.h"
#include "s_table.h"

s_attribute* Stable::add_local(const std::string& name, const s_attribute& attribute)
{
    if (localtables.back().map.find(name) != localtables.back().map.end())return nullptr;
    return &( localtables.back().map.insert(
        std::make_pair(std::string(name), s_attribute(attribute))).first->second );
}

s_attribute* Stable::add_global(const std::string& name, const s_attribute& attribute)
{
    if (global_defs.find(name) != global_defs.end())return nullptr;
    return &global_defs.insert(std::make_pair(std::string(name), s_attribute(attribute))).first->second;
}

void Stable::pop()
{
    if (!localtables.empty())localtables.pop_back();
}

void Stable::push(int local_sp_addr_offset)
{
    localtables.push_back(sTableEntry());
    localtables.back().frame_sp_base = local_sp_addr_offset;
}

const s_attribute* Stable::find(const std::string& name)
{
    auto x = localtables.rbegin();
    while (x != localtables.rend()) {
        auto f = x->map.find(name);
        if (f != x->map.end())return &f->second;
    }
    auto f = global_defs.find(name);
    if (f != global_defs.end())return &f->second;
    if (_func_var_p) { auto res = _func_var_p->find(name);  if (res != (_func_var_p->end()))return & res->second; }
    return nullptr;
}

Stable::Stable()
{
    cnttypeida = 1;
    _type_description* p = new _type_description;
    p->name = "int";
    p->size = 4;
    p->is_struct = false;
    add_ty(p);
    p = new _type_description;
    p->name = "short";
    p->size = 2;
    p->is_struct = false;
    add_ty(p);
    p = new _type_description;
    p->name = "char";
    p->size = 1;
    p->is_struct = false;
    add_ty(p);
}

Stable::~Stable()
{
    for (auto a : _tytkbyid) {
        delete (a.second);
    }
}

int Stable::add_ty(_type_description* d)
{
    d->id = this->cnttypeida;
    this->_tytkbyid[cnttypeida] = d;
    this->_tytkbyname.insert(std::make_pair(d->name, d));
    cnttypeida++;
    return cnttypeida - 1;
}

void Stable::parse_func(const std::string& name)
{
    auto fn = global_defs.find(name);
    if (fn != global_defs.end()) {
        this->_func_var_p = &(fn->second.func_args->first);
    }
}

_type_description* Stable::find_type_by_id(int id)
{
    auto a = _tytkbyid.find(id);
    if (a != _tytkbyid.end())return a->second;
    return nullptr;
}

_type_description* Stable::find_type_by_name(const std::string& name)
{
    auto a = _tytkbyname.find(name);
    if (a != _tytkbyname.end())return a->second;
    return nullptr;
}

s_attribute::~s_attribute()
{
    if (s_class == Description::Function && func_args != nullptr)
        delete func_args;
}

s_attribute::s_attribute(const s_attribute&b)
{
    this->attr = b.attr;
    this->local_addr_offset = b.local_addr_offset;
    this->size = b.size;
    this->s_class = b.s_class;
    this->Type = b.Type;
    if (b.s_class == Description::Function) {
        func_args = new std::pair<std::map<std::string, s_attribute>, std::list<s_attribute*> >(*b.func_args);
    }
    else func_args = nullptr; 

}
