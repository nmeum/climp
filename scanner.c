#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/queue.h>

#include "scanner.h"
#include "util.h"

static token septok = { .type = -1, .text = NULL, .line = -1 };

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

	dest = emalloc(siz * sizeof(char*));
	tok = emalloc(sizeof(*tok));

	strncpy(dest, &scr->input[scr->start], siz - 1);
	dest[siz - 1] = '\0';

	tok->line = scr->line;
	tok->text = dest;
	tok->type = tkt;

	sem_wait(scr->emptysem);
	pthread_mutex_lock(scr->qmutex);
	TAILQ_INSERT_BEFORE(&septok, tok, toks);
	pthread_mutex_unlock(scr->qmutex);
	sem_post(scr->fullsem);

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
	dest = emalloc(slen * sizeof(char*));

	vsnprintf(dest, slen, msg, ap);
	va_end(ap);

	tok = emalloc(sizeof(*tok));
	tok->line = scr->start;
	tok->text = dest;
	tok->type = TOK_ERROR;

	sem_wait(scr->emptysem);
	pthread_mutex_lock(scr->qmutex);
	TAILQ_INSERT_BEFORE(&septok, tok, toks);
	pthread_mutex_unlock(scr->qmutex);
	sem_post(scr->fullsem);

	scr->start = scr->pos;
}

void*
lexany(void *pscr)
{
	scanner *scr = (scanner*)pscr;
	char nxt;

	if ((nxt = nextch(scr)) == -1) {
		emit(scr, TOK_EOF);
		return NULL;
	}
	
	switch (nxt) {
	case '\n':
		scr->line++;
		ignore(scr);
		lexany(scr);
		return NULL;
	case ';':
		emit(scr, TOK_SEMICOLON);
		lexany(scr);
		return NULL;
	case ':':
		lexassign(scr);
		return NULL;
	case '(':
		emit(scr, TOK_LBRACKET);
		lexany(scr);
		return NULL;
	case ')':
		emit(scr, TOK_RBRACKET);
		lexany(scr);
		return NULL;
	case '%':
		emit(scr, TOK_DIVIDE);
		lexany(scr);
		return NULL;
	case '*':
		emit(scr, TOK_MULTI);
		lexany(scr);
		return NULL;
	case '+':
		emit(scr, TOK_PLUS);
		lexany(scr);
		return NULL;
	case '-':
		emit(scr, TOK_MINUS);
		lexany(scr);
		return NULL;
	case '?':
		emit(scr, TOK_QUESTION);
		lexany(scr);
		return NULL;
	case '!':
		emit(scr, TOK_EXCLAMATION);
		lexany(scr);
		return NULL;
	}

	if (isalpha(nxt)) {
		lexvar(scr);
		return NULL;
	} else if (isdigit(nxt)) {
		lexdigit(scr);
		return NULL;
	} else if (isspace(nxt)) {
		lexspace(scr);
		return NULL;
	}

	errf(scr, "Invalid character '%c'", nxt);
	return NULL;
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
	token *tok, *nxt;

	if (!scr)
		return;

	pthread_join(scr->thread, NULL); /* TODO stop the thread instead. */
	pthread_mutex_destroy(scr->qmutex);

	TAILQ_FOREACH_SAFE(tok, &scr->qhead, toks, nxt) {
		if (tok->type != -1) {
			free(tok->text);
			free(tok);
		}
	}

	if (sem_destroy(scr->fullsem) ||
			sem_destroy(scr->emptysem))
		die("sem_destroy failed");

	free(scr->fullsem);
	free(scr->emptysem);
	free(scr->qmutex);
	free(scr->input);
	free(scr);
}

scanner*
scanstr(char *str)
{
	scanner *scr;

	scr = emalloc(sizeof(*scr));
	scr->eof    = 0;
	scr->pos    = 0;
	scr->start  = 0;
	scr->input  = estrdup(str);
	scr->inlen  = strlen(str);
	scr->qmutex = emalloc(sizeof(pthread_mutex_t));

	scr->fullsem  = emalloc(sizeof(sem_t));
	scr->emptysem = emalloc(sizeof(sem_t));

	/* TODO adjust initial emptysem value */
	if (sem_init(scr->fullsem, 0, 0)
			|| sem_init(scr->emptysem, 0, 250))
		die("sem_init failed");

	TAILQ_INIT(&scr->qhead);
	TAILQ_INSERT_TAIL(&scr->qhead, &septok, toks);

	pthread_mutex_init(scr->qmutex, NULL);
	pthread_create(&scr->thread, NULL, lexany, scr);
	return scr;
}

token*
nxttok(scanner *scr)
{
	token *tok;

	if (scr->eof) return NULL;

	sem_wait(scr->fullsem);
	pthread_mutex_lock(scr->qmutex);
	tok = TAILQ_FIRST(&scr->qhead);
	TAILQ_REMOVE(&scr->qhead, tok, toks);
	TAILQ_INSERT_AFTER(&scr->qhead, &septok, tok, toks);
	pthread_mutex_unlock(scr->qmutex);
	sem_post(scr->emptysem);

	if (tok->type == TOK_EOF)
		scr->eof = 1;

	return tok;
}
