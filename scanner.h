typedef enum {
	EOT,     /* End of termination */
	ERROR,   /* Lexer error of some sort */
	NEWLINE, /* The newline character */
} tok_t;

typedef struct token token;

struct token {
	SIMPLEQ_ENTRY(token) toks;
	tok_t type;
	int line;
};

SIMPLEQ_HEAD(tqueue, token);

typedef struct scanner scanner;

struct scanner {
	void (*state)(scanner *scr);
	size_t pos, start, inlen;
	struct tqueue qhead;
	char *input;
	int line;
};
