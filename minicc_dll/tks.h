#ifndef TKS_H_ 
#define TKS_H_
///C style header for token...

typedef struct String_
{
	char* str;
	unsigned int length;
	unsigned int capacity;
}String;

int init_string_(String* str, const char* pstrart, unsigned int length);

void destruct_string_(String* str);

#define TK_TYPE_ID 1
#define TK_TYPE_OP 2
#define TK_TYPE_STOP 3
#define TK_TYPE_KEYWORD 4
#define TK_TYPE_NUMBER 5
#define TK_TYPE_STRING 6

#define STREAM_DEF_CAPACITY 8192

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

Token* Tokenlize(const char* str, unsigned int length);

Stream* CreateStream(Stream* head, unsigned stream_length);

void destruct_stream(Stream* stream);


#endif