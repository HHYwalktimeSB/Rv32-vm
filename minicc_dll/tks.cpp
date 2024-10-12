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

#define Stream_is_HEAD(_FLAG_) ((_FLAG_&2))

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

#define CTY_OP 1
#define CTY_LETTER 2
#define CTY_STOP 3
#define CTY_STR 4
#define CTY_NUM 5
#define CTY_SPACE 6
#define CTY_UNKNOW 0
#define CTY_TABLE_MASK 127
#define CTY_FLAG_SPECIAL 7

const char CTYtable[128] = {6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6, 
    6,1,4,0,0,1,1,4         , 3,3,1,1,3,1,3,1,      5,5,5,5,5, 5,5,5,5,5,       3,3,1,1,1,1,    
    0,  2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2,2,    1,7,1,1,2,
    0,  2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2, 2,2,2,2,2,   2,2,2,2,2,2,    3,1,3,0,0};

#define STATE_READ_OP
#define STATE_INIT
#define STATE_READ_ID
#define STATE_READ_STR
#define STATE_READ_NUMBER

Token* Tokenlize(const char* str, unsigned int length)
{
    return nullptr;
}

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
