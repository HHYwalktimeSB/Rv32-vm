#include "pch.h"
#include "tks.h"
#include<stdlib.h>
#include<string.h>

int init_string_(String* str, const char* pstrart, unsigned int length)
{
    if (length == 0) length = strlen(pstrart);
    str->str = (char*)malloc(length + 1);
    if (str->str == nullptr)return -1;
    str->capacity = str->length = length;
    memcpy(str->str, pstrart, length);
    str->str[length] = '\0';
    return 0;
}

void destruct_string_(String* str)
{
    if (!str || !(str->str))return;
    free(str->str);
}

int WriteStream(Stream* ptr, const char* Src, unsigned length)
{
    while (ptr->next != NULL)ptr = ptr->next;//find tail 
    unsigned length_write = ptr->capacity - ptr->size;
    if (length_write > length)length_write = length;
    memcpy(ptr->data + ptr->size, Src, length_write);//cp1nd
    ptr->size += length_write;
    if (length == length_write) {
        if (ptr->capacity == ptr->size)
            ptr->next = CreateStream((ptr->flag & 2) == 0 ? ptr->head : ptr, STREAM_DEF_CAPACITY);
        return 0;
    }
    length -= length_write;
    Src += length_write;
    ptr->next = CreateStream((ptr->flag & 2) == 0 ? ptr->head : ptr, (length&0x1FFF) + 8192);
    memcpy(ptr->data, Src, length);
    ptr->size = length;
    return 0;
}

unsigned int ReadStream(Stream* ptr, char* buf, unsigned length)
{
    unsigned cbread = 0;
    unsigned cbcopy;
    Stream* head = (ptr->flag) ? ptr : ptr->head;
    ptr = head->head;
    while (length >0 && ptr != NULL) {
        cbcopy = ptr->size - ptr->ioptr;
        if (cbcopy > length)cbcopy = length;
        memcpy(buf, ptr->data + ptr->ioptr, cbcopy);
        buf += cbcopy;
        ptr->ioptr += cbcopy;
        cbread += cbcopy;
        length -= cbcopy;
        if (ptr->ioptr == ptr->size) {
            head->head = ptr->next;//ÅäÖÃÐÅÏ¢ÐÞ¸Ä
            ptr = ptr->next;
            if (ptr)ptr->ioptr = 0;
        }
    }
    return cbread;
}

int StreamPut(Stream* ptr, char c)
{
    while (ptr->next != NULL)ptr = ptr->next;
    if (ptr->capacity <= ptr->size) {
        ptr->next = CreateStream((ptr->flag) ? ptr : ptr->head, STREAM_DEF_CAPACITY);
        ptr = ptr->next;
    }
    ptr->data[ptr->size] = c;
    ptr->size += 1;
    return 0;
}

unsigned short StreamGet(Stream* ptr)
{
    if (ptr->flag == 0)ptr = ptr->head;
    if (ptr->head->ioptr >= ptr->head->size) {
        ptr->head = ptr->head->next;
        if (ptr->head == NULL)return 1;
    }
    return ((unsigned short)(ptr->head->data[(ptr->head->ioptr)++]))<<8;
}

#define CTY_OP 1
#define CTY_LETTER 2
#define CTY_STOP 3
#define CTY_STR 4
#define CTY_NUM 5
#define CTY_SPACE 6
#define CTY_UNKNOW 0
#define CTY_TABLE_MASK 127
#define CTY_FLAG_SPECIAL 7
#define CTY_NEWLINE 9

const char CTYtable[128] = {6,6,6,6,6,6,6,6  ,6,6,9,6,6,6,6,6 ,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6, 
    6,1,4,0,0,1,1,4         , 3,3,1,1,3,1,3,1,      5,5,5,5,5, 5,5,5,5,5,       3,3,1,1,1,1,    
    0,  2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2,2,    1,7,1,1,2,
    0,  2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2,2,    3,1,3,0,0};

#define STATE_READ_OP 1
#define STATE_INIT 2
#define STATE_READ_ID 3
#define STATE_READ_STR 4
#define STATE_READ_NUMBER 5
#define STATE_READ_COMMENT 6

Stream* CreateStream(Stream* head, unsigned stream_length)
{
    Stream* ret = (Stream*)malloc(sizeof(Stream));
    if (ret == NULL)return NULL;
    if (head) {
        ret->head = head;
        *const_cast<unsigned int*>(&ret->flag) = 0;
    }
    else {
        *const_cast<unsigned int*>(&ret->flag) = 2;
        ret->head = ret;
    }
    ret->ioptr = 0;
    ret->capacity = stream_length;
    ret->size = 0;
    ret->next = NULL;
    ret->data = (char*)malloc(ret->capacity);
    if (ret->data == nullptr)free(ret), ret = nullptr;
    return ret;
}

void destruct_stream_helper_(Stream* stream) {
    if (stream->next != NULL)destruct_stream_helper_(stream->next);
    //destruct this node
    if (stream->data != 0)free(stream->data);
    free(stream);
}

void destruct_stream(Stream* stream)
{
    if ((stream->flag & 2)==0) {
        destruct_stream_helper_(stream->head);
    }
    else {
        destruct_stream_helper_(stream);
    }
}

Token* append_next_tk(Token* prev, const char* str, unsigned strlength, int type) {
    Token* papp = (Token*)malloc(sizeof(Token));
    if (papp == NULL)return nullptr;
    init_string_(&papp->str, str, strlength);
    papp->val = 0;
    papp->next = nullptr;
    papp->type = type;
    prev->next = papp;
    return papp;
}
#define MAKE_THROW_FOR_UNKNOWN_CHARS throw "using character(s) that are not supported"

#include<string>
#include<unordered_map>

std::unordered_map<std::string, int>keyword_map;

void init_keyword_map() {
    keyword_map.insert(std::make_pair(std::string("int"), KW_TYPENAME_INT));
    keyword_map.insert(std::make_pair(std::string("unsigned"), KW_TYPENAME_D_UNSIGNED));
    keyword_map.insert(std::make_pair(std::string("short"), KW_TYPENAME_SHORT));
    keyword_map.insert(std::make_pair(std::string("char"), KW_TYPENAME_CHAR));
    keyword_map.insert(std::make_pair(std::string("const"), KW_TYPENAME_D_CONST));
    keyword_map.insert(std::make_pair(std::string("return"), KW_CTRL_RETURN));
    keyword_map.insert(std::make_pair(std::string("if"), KW_CTRL_IF));
    keyword_map.insert(std::make_pair(std::string("while"), KW_CTRL_WHILE));
    keyword_map.insert(std::make_pair(std::string("for"), KW_CTRL_FOR));
    keyword_map.insert(std::make_pair(std::string("else"), KW_CTRL_ELSE));
    keyword_map.insert(std::make_pair(std::string("continue"), KW_CTRL_CONTINUE));
    keyword_map.insert(std::make_pair(std::string("break"), KW_CTRL_BREAK));
    keyword_map.insert(std::make_pair(std::string("struct"), KW_STRUCT));
    keyword_map.insert(std::make_pair(std::string("eunm"), KW_ENUM));
    keyword_map.insert(std::make_pair(std::string("do"), KW_CTRL_DO));
}

static inline void FSM_transform_state(unsigned & state, unsigned &flag, unsigned state_transform_to, std::string& strbuf,
    Token*& cur, char c) {
    switch (state)
    {
    case STATE_INIT:
        switch (CTYtable[c & CTY_TABLE_MASK])
        {
        case CTY_OP:
            state_transform_to = STATE_READ_OP;
            break;
        case CTY_LETTER:
            state_transform_to = STATE_READ_ID;
            break;
        case CTY_NUM:
            state_transform_to = STATE_READ_NUMBER;
            break;
        case CTY_SPACE:
        case CTY_NEWLINE:
        case CTY_FLAG_SPECIAL:
            state_transform_to = state;
            break;
        case CTY_STR:
            state_transform_to = STATE_READ_STR;
            break;
        case CTY_STOP:
            cur = append_next_tk(cur, &c, 1, TK_TYPE_STOP);
            state_transform_to = state;
            break;
        case CTY_UNKNOW:
            MAKE_THROW_FOR_UNKNOWN_CHARS;
            break;
        }
        if (state_transform_to != state) {
            strbuf.clear();
            strbuf.push_back(c);
            state = state_transform_to;
        }
        break;
    case STATE_READ_ID:
        switch (CTYtable[c & CTY_TABLE_MASK])
        {
        case CTY_LETTER:
        case CTY_NUM:
            strbuf.push_back(c);
            state_transform_to = state;
            break;
        case CTY_SPACE:
        case CTY_NEWLINE:
        case CTY_FLAG_SPECIAL:
            state_transform_to = STATE_INIT;
            break;
        case CTY_OP:
            state_transform_to = STATE_READ_OP;
            break;
        case CTY_STR:
            state_transform_to = STATE_READ_STR;
            break;
        case CTY_STOP:
            state_transform_to = state = STATE_INIT;
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_ID);
            strbuf.clear();
            cur = append_next_tk(cur, &c, 1, TK_TYPE_STOP);
            break;
        }
        if (state_transform_to != state) {
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_ID);
            strbuf.clear();
            strbuf.push_back(c);
            state = state_transform_to;
        }
        break;
    case STATE_READ_NUMBER:
        switch (CTYtable[c & CTY_TABLE_MASK])
        {
        case CTY_OP:
            state_transform_to = STATE_READ_OP;
            break;
        case CTY_STR:
            state_transform_to = STATE_READ_STR;
            break;
        case CTY_LETTER:
            state_transform_to = STATE_READ_ID;
            break;
        case CTY_NUM:
            strbuf.push_back(c);
            state_transform_to = state;
            break;
        case CTY_SPACE:
        case CTY_FLAG_SPECIAL:
            state_transform_to = STATE_INIT;
            break;
        case CTY_STOP:
            state_transform_to = state = STATE_INIT;
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_NUMBER);
            strbuf.clear();
            cur = append_next_tk(cur, &c, 1, TK_TYPE_STOP);
            break;
        }
        if (state_transform_to != state) {
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_NUMBER);
            strbuf.clear();
            strbuf.push_back(c);
            state = state_transform_to;
        }
        break;
    case STATE_READ_STR:
        strbuf.push_back(c);
        switch (CTYtable[c & CTY_TABLE_MASK])
        {
        case CTY_NEWLINE:
            flag = 0;
            strbuf.pop_back();
            break;
        case CTY_FLAG_SPECIAL:
            flag = 2;
        case CTY_OP:
        case CTY_LETTER:
        case CTY_NUM:
        case CTY_SPACE:
        case CTY_STOP:
            flag >>= 1;
            break;
        case CTY_STR:
            if (flag == 0) {
                state = STATE_INIT;
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_STRING);
                strbuf.clear();
            }
            else flag = 0;
            break;
        }
        break;
    case STATE_READ_OP:
        switch (CTYtable[c & CTY_TABLE_MASK])
        {
        case CTY_OP:
            if (c == '/' && strbuf[0] == '/') {
                state_transform_to = STATE_READ_COMMENT;
                break;
            }
            if (c == '+' && strbuf[0] == '+') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_INC;
                strbuf.clear();
                break;
            }
            if (c == '-' && strbuf[0] == '-') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_DEC;
                strbuf.clear(); 
                break;
            }
            if (c == '|' && strbuf[0] == '|') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_OR;
                strbuf.clear();
                break;
            }
            if (c == '&' && strbuf[0] == '&') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_AND;
                strbuf.clear();
                break;
            }
            if (c == '>' && strbuf[0] == '-') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_PTRACCEES;
                strbuf.clear();
                break;
            }
            if (c == '>' && strbuf[0] == '>') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_RS;
                strbuf.clear();
                break;
            }
            if (c == '<' && strbuf[0] == '<') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_LS;
                strbuf.clear();
                break;
            }
            if (c == '=') {
                state = state_transform_to = STATE_INIT;
                strbuf.push_back(c);
                cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
                cur->val = TKV_OP_PLACEHOLDER_ASSIGN;
                strbuf.clear();
                break;
            }
            state_transform_to = state;
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
            strbuf.clear();
            strbuf.push_back(c);
            break;
        case CTY_LETTER:
            state_transform_to = STATE_READ_ID;
            break;
        case CTY_NUM:
            state_transform_to = STATE_READ_NUMBER;
            break;
        case CTY_SPACE:
        case CTY_FLAG_SPECIAL:
            state_transform_to = STATE_INIT;
            break;
        case CTY_STR:
            state_transform_to = STATE_READ_STR;
            break;
        case CTY_STOP:
            state_transform_to = state = STATE_INIT;
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
            strbuf.clear();
            cur = append_next_tk(cur, &c, 1, TK_TYPE_STOP);
            break;
        }
        if (state_transform_to != state) {
            cur = append_next_tk(cur, strbuf.c_str(), strbuf.length(), TK_TYPE_OP);
            strbuf.clear();
            strbuf.push_back(c);
            state = state_transform_to;
        }
        break;
    case STATE_READ_COMMENT:
        if (c == '\n') {
            state = STATE_INIT;
            strbuf.clear();
        }
        break;
    }
}

Token* Tokenlize(Stream* stream)
{
    Token head;
    unsigned short cchret;
    unsigned int state=STATE_INIT,flag=0;
    unsigned state_transform_to = 0;
    std::string strbuf;
    Token* cur = &head;
    while (((cchret = StreamGet(stream))&CTY_TABLE_MASK)==0) {
        FSM_transform_state(state, flag, state_transform_to, strbuf, cur, char(cchret >> 8));
    }
    if (state != STATE_INIT)return 0;
    return head.next;
}

MY_API Token* Tokenlize(const char* str, unsigned int length)
{
    Token head;
    unsigned int state = STATE_INIT, flag = 0;
    unsigned state_transform_to = 0;
    std::string strbuf;
    Token* cur = &head;
    unsigned x = 0;
    decltype (keyword_map)::iterator iter;
    while (x < length) {
        FSM_transform_state(state, flag, state_transform_to, strbuf, cur, str[x]);
        ++x;
    }
    cur = head.next;
    while (cur != NULL) {
        if (cur->type == TK_TYPE_ID) {
            strbuf.assign(cur->str.str, cur->str.length);
            iter = keyword_map.find(strbuf);
            if (iter != keyword_map.end()) {
                cur->type = TK_TYPE_KEYWORD;
                cur->val = iter->second; 
            }
        }
        cur = cur->next;
    }
    return head.next;
}

MY_API Token* print_tk(Token* tkhead)
{
    while (tkhead->next != nullptr) {
        printf("%s, %d\n", tkhead->str.str, tkhead->type);
        tkhead = tkhead->next;
    }
    printf("%s\n", tkhead->str.str);
    printf("end\n");
    return 0;
}
