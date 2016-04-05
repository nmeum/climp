#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/queue.h>

#include "scanner.h"
#include "climp.h"

void lexvar(scanner *scr);
void lexdigit(scanner *scr);
void lexspace(scanner *scr);
void lexassign(scanner *scr);

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
	if (nxt != -1) scr->pos--;
	return nxt;
}

void
ignore(scanner *scr)
{
	scr->start = scr->pos;
}

void
emit(scanner *scr, tok_t tkt)
{
	size_t siz = scr->pos - scr->start + 1;
	token *tok;
	char *dest;

	if (!(dest = malloc(siz * sizeof(char*))))
		die("malloc failed");

	if (!(tok = malloc(sizeof(*tok))))
		die("malloc failed");

	strncpy(dest, &scr->input[scr->start], siz - 1);
	dest[siz - 1] = '\0';

	if (tkt == TOK_NEWLINE)
		scr->line++;

	tok->line = scr->line;
	tok->text = dest;
	tok->type = tkt;

	SIMPLEQ_INSERT_TAIL(&scr->qhead, tok, toks);
	scr->start = scr->pos;
}

void
errf(scanner *scr, char *msg, ...)
{
	int slen = 1;
	va_list ap;
	token *tok;
	char *dest;

	va_start(ap, msg);
	slen += vsnprintf(NULL, 0, msg, ap);
	if (!(dest = malloc(slen * sizeof(char*))))
		die("malloc failed");

	vsnprintf(dest, slen, msg, ap);
	va_end(ap);

	if (!(tok = malloc(sizeof(*tok))))
		die("malloc failed");

	tok->line = scr->start;
	tok->text = dest;
	tok->type = TOK_ERROR;

	SIMPLEQ_INSERT_TAIL(&scr->qhead, tok, toks);
	scr->start = scr->pos;
}

void
lexany(scanner *scr)
{
	char nxt;

	if ((nxt = nextch(scr)) == -1)
		return;
	
	switch (nxt) {
	case '\n':
		emit(scr, TOK_NEWLINE);
		lexany(scr);
		return;
	case ';':
		emit(scr, TOK_SEMICOLON);
		lexany(scr);
		return;
	case ':':
		lexassign(scr);
		return;
	case '(':
		emit(scr, TOK_LBRACKET);
		lexany(scr);
		return;
	case ')':
		emit(scr, TOK_RBRACKET);
		lexany(scr);
		return;
	case '%':
		emit(scr, TOK_DIVIDE);
		lexany(scr);
		return;
	case '*':
		emit(scr, TOK_MULTI);
		lexany(scr);
		return;
	case '+':
		emit(scr, TOK_PLUS);
		lexany(scr);
		return;
	case '-':
		emit(scr, TOK_MINUS);
		lexany(scr);
		return;
	case '?':
		emit(scr, TOK_QUESTION);
		lexany(scr);
		return;
	case '!':
		emit(scr, TOK_EXCLAMATION);
		lexany(scr);
		return;
	}

	if (isalpha(nxt)) {
		lexvar(scr);
		return;
	} else if (isdigit(nxt)) {
		lexdigit(scr);
		return;
	} else if (isspace(nxt)) {
		lexspace(scr);
		return;
	}

	errf(scr, "Invalid character '%c'", nxt);
}

void
lexassign(scanner *scr) {
	char c;

	if ((c = peekch(scr)) == '=') {
		nextch(scr);
		emit(scr, TOK_ASSIGN);
	} else {
		errf(scr, "Expected '=' after ':' got '%c'", c);
	}

	lexany(scr);
}

void
lexvar(scanner *scr)
{
	while (isalpha(peekch(scr)))
		nextch(scr);

	emit(scr, TOK_VAR);
	lexany(scr);
}

void
lexdigit(scanner *scr)
{
	while (isdigit(peekch(scr)))
		nextch(scr);

	emit(scr, TOK_DIG);
	lexany(scr);
}

void
lexspace(scanner *scr)
{
	while (isspace(peekch(scr)))
		nextch(scr);

	ignore(scr);
	lexany(scr);
}

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
	scr->start = 0;
	scr->input = str;
	scr->inlen = strlen(str);

	SIMPLEQ_INIT(&scr->qhead);
	lexany(scr);

	return scr;
}

token*
nxttok(scanner *scr)
{
	token *tok;

	if (SIMPLEQ_EMPTY(&scr->qhead))
		return NULL;

	tok = SIMPLEQ_FIRST(&scr->qhead);
	SIMPLEQ_REMOVE_HEAD(&scr->qhead, toks);

	return tok;
}
