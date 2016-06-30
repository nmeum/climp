typedef enum {
	TOK_EOF,         /* End of file */

	TOK_VAR,         /* A variable name */
	TOK_DIG,         /* A natural number */
	TOK_ERROR,       /* Lexer error of some sort */

	TOK_ASSIGN,      /* := */
	TOK_QUESTION,    /* ? */
	TOK_SEMICOLON,   /* ; */
	TOK_EXCLAMATION, /* ! */

	TOK_LBRACKET,    /* ( */
	TOK_RBRACKET,    /* ) */

	TOK_DIVIDE,      /* % */
	TOK_MULTI,       /* * */
	TOK_MINUS,       /* - */
	TOK_PLUS,        /* + */
} tok_t;

typedef struct token token;

struct token {
	TAILQ_ENTRY(token) toks;
	tok_t type;
	char *text;
	int line;
};

TAILQ_HEAD(tqueue, token);

typedef struct scanner scanner;

struct scanner {
	sem_t *fullsem, *emptysem;
	pthread_t *thread;
	size_t pos, start, inlen;
	struct tqueue qhead;
	pthread_mutex_t *qmutex;
	char *input;
	int line, eof;
};

void freescr(scanner *scr);
scanner *scanstr(char *str);
token *nxttok(scanner *scr);
