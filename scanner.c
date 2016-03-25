#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include <sys/queue.h>

#define die(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef enum {
	EOT,     /* End of termination */
	ERROR,   /* Lexer error of some sort */
	NEWLINE, /* The newline character */
} tok_t;

typedef struct token token;

struct token {
	tok_t type;
	int line;
	SIMPLEQ_ENTRY(token) toks;
};

SIMPLEQ_HEAD(listhead, token) qhead = SIMPLEQ_HEAD_INITIALIZER(qhead);

typedef struct scanner scanner;

struct scanner {
	void (*state)(scanner *scr);
	size_t pos, start, inlen;
	char *input;
	int line;
};

scanner*
scanstr(char *str)
{
	scanner *scr;
		
	if (!(scr = malloc(sizeof(*scr))))
		die("malloc failed");

	scr->pos   = 0;
	scr->input = str;
	scr->inlen = strlen(str);

	return scr;
}

char
nextch(scanner *scr)
{
	if (scr->pos >= scr->inlen)
		return -1;

	return scr->input[scr->pos++];
}

char
peekch(scanner *scr)
{
	char nxt = nextch(scr);

	scr->pos--;
	return nxt;
}

void
emit(scanner *scr, tok_t tkt)
{
	size_t siz = scr->pos - scr->start;
	char buf[siz];
	token *tok;

	if (!(tok = malloc(sizeof(*tok))))
		die("malloc failed");

	if (tkt == NEWLINE) scr->line++;
	memcpy(buf, scr, siz);

	tok->line = scr->line;
	tok->type = tkt;

	SIMPLEQ_INSERT_TAIL(&qhead, tok, toks);
}

void
lexany(scanner *scr)
{
	char nxt;

	if ((nxt = nextch(scr)) == -1)
		emit(scr, ERROR);
	
	if (nxt == '\n')
		emit(scr, NEWLINE);

	printf("%d\n", SIMPLEQ_FIRST(&qhead)->line);
}

int
main(void)
{
	scanner *scr;

	scr = scanstr("\n");
	lexany(scr);

	free(scr);
	return 0;
}
