#ifndef TKS_H_ 
#define TKS_H_
///C style header for token...

#ifdef _DLL

#define MY_API extern "C" __declspec(dllexport)

#else

#define MY_API extern "C"

#endif

typedef struct String_
{
	char* str;
	unsigned int length;
	unsigned int capacity;
}String;

int init_string_(String* str, const char* pstrart, unsigned int length);

void destruct_string_(String* str);

void init_keyword_map();

#define TK_TYPE_ID 1
#define TK_TYPE_OP 2
#define TK_TYPE_STOP 3
#define TK_TYPE_KEYWORD 4
#define TK_TYPE_NUMBER 5
#define TK_TYPE_STRING 6

#define STREAM_DEF_CAPACITY 8192

#define TKV_OP_INC 1
#define TKV_OP_DEC 2

#define TKV_OP_PTR 3
#define TKV_OP_NORMAL 0

#define TKV_OP_AND 4
#define TKV_OP_OR 5

#define TKV_OP_PLACEHOLDER_ASSIGN 114

typedef struct Token_ {
	String str;
	int val;
	int type;
	struct Token_* next;
}Token;

typedef struct Stream_ {
	char* data;
	unsigned size;
	unsigned capacity;
	struct Stream_* next;
	unsigned ioptr;
	const unsigned flag;//is currently read 1, is head 2; 
	struct Stream_* head;//for head, stores the current io stream
}Stream;

int WriteStream(Stream* ptr, const char* Src, unsigned length);

unsigned int ReadStream(Stream* ptr, char* buf, unsigned length);

int StreamPut(Stream* ptr, char c);

unsigned short StreamGet(Stream* ptr);

Stream* CreateStream(Stream* head, unsigned stream_length);

void destruct_stream(Stream* stream);

Token* Tokenlize(Stream* stream);

MY_API Token* Tokenlize(const char* str, unsigned int length);

MY_API Token* print_tk(Token* tkhead);

#define KW_TYPENAME_INT 16
#define KW_TYPENAME_SHORT 15
#define KW_TYPENAME_D_CONST 14
#define KW_TYPENAME_D_UNSIGNED 13
#define KW_TYPENAME_CHAR 12 
#define KW_ENUM 32
#define KW_STRUCT 31
#define KW_CTRL_FOR 30
#define KW_CTRL_WHILE 29
#define KW_CTRL_DO 28
#define KW_CTRL_IF 27
#define KW_CTRL_ELSE 26
#define KW_CTRL_RETURN 1
#define KW_CTRL_BREAK 2
#define KW_CTRL_CONTINUE 3
#define KW_CTRL_GOTO 4

#endif