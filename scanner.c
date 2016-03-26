#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include <sys/queue.h>

#include "scanner.h"
#include "climp.h"

void
freescr(scanner *scr)
{
	token *tok;

	if (!scr) return;
	SIMPLEQ_FOREACH(tok, &scr->qhead, toks)
		free(tok);

	free(scr);
}

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
	char buf[siz + 1];
	token *tok;

	if (!(tok = malloc(sizeof(*tok))))
		die("malloc failed");

	memcpy(buf, &scr->input[scr->start], siz);
	buf[++siz] = '\0';

	printf("%s", buf);

	if (tkt == NEWLINE)
		scr->line++;

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

	freescr(scr);
	return 0;
}
