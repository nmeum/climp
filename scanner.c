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
	SIMPLEQ_FOREACH(tok, &scr->qhead, toks) {
		if (tok->text) free(tok->text);
		free(tok);
	}

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
	size_t siz = scr->pos - scr->start + 1;
	token *tok;
	char *dest;

	if (!(dest = malloc(siz + 1 * sizeof(char*))))
		die("malloc failed");

	if (!(tok = malloc(sizeof(*tok))))
		die("malloc failed");

	memcpy(dest, &scr->input[scr->start], siz);
	dest[siz + 1] = '\0';

	if (tkt == NEWLINE)
		scr->line++;

	tok->line = scr->line;
	tok->text = dest;
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

	printf("%s", SIMPLEQ_FIRST(&scr->qhead)->text);
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
