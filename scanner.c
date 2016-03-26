#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include <sys/queue.h>

#include "scanner.h"
#include "climp.h"

scanner*
scanstr(char *str)
{
	scanner *scr;
		
	if (!(scr = malloc(sizeof(*scr))))
		die("malloc failed");

	scr->pos   = 0;
	scr->input = str;
	scr->inlen = strlen(str);

	SIMPLEQ_INIT(&scr->qhead);
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

	SIMPLEQ_INSERT_TAIL(&scr->qhead, tok, toks);
	scr->start = scr->pos;
}

void
lexany(scanner *scr)
{
	char nxt;

	if ((nxt = nextch(scr)) == -1)
		emit(scr, ERROR);
	
	if (nxt == '\n')
		emit(scr, NEWLINE);

	printf("%d\n", SIMPLEQ_FIRST(&scr->qhead)->line);
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
