typedef enum {
	TOK_EOF,         /* End of file */
	TOK_VAR,         /* A variable name */
	TOK_ERROR,       /* Lexer error of ome sort */
	TOK_NEWLINE,     /* The newline character */
	TOK_QUESTION,    /* ? */
	TOK_SEMICOLON,   /* ; */
	TOK_EXCLAMATION, /* ! */
} tok_t;

typedef struct token token;

struct token {
	SIMPLEQ_ENTRY(token) toks;
	tok_t type;
	char *text;
	int line;
};

SIMPLEQ_HEAD(tqueue, token);

typedef struct scanner scanner;

struct scanner {
	size_t pos, start, inlen;
	struct tqueue qhead;
	char *input;
	int line;
};

void freescr(scanner *scr);
scanner *scanstr(char *str);
token *nxttok(scanner *scr);
