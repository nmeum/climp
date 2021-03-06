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

void* lexvar(scanner *scr);
void* lexdigit(scanner *scr);
void* lexspace(scanner *scr);
void* lexassign(scanner *scr);

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
		return lexany(scr);
	case ';':
		emit(scr, TOK_SEMICOLON);
		return lexany(scr);
	case ':':
		return lexassign(scr);
	case '(':
		emit(scr, TOK_LBRACKET);
		return lexany(scr);
	case ')':
		emit(scr, TOK_RBRACKET);
		return lexany(scr);
	case '%':
		emit(scr, TOK_DIVIDE);
		return lexany(scr);
	case '*':
		emit(scr, TOK_MULTI);
		return lexany(scr);
	case '+':
		emit(scr, TOK_PLUS);
		return lexany(scr);
	case '-':
		emit(scr, TOK_MINUS);
		return lexany(scr);
	case '?':
		emit(scr, TOK_QUESTION);
		return lexany(scr);
	case '!':
		emit(scr, TOK_EXCLAMATION);
		return lexany(scr);
	}

	if (isalpha(nxt)) {
		return lexvar(scr);
	} else if (isdigit(nxt)) {
		return lexdigit(scr);
	} else if (isspace(nxt)) {
		return lexspace(scr);
	}

	errf(scr, "Invalid character '%c'", nxt);
	return NULL;
}

void*
lexassign(scanner *scr) {
	char c;

	if ((c = peekch(scr)) == '=') {
		nextch(scr);
		emit(scr, TOK_ASSIGN);
	} else {
		errf(scr, "Expected '=' after ':' got '%c'", c);
	}

	return lexany(scr);
}

void*
lexvar(scanner *scr)
{
	while (isalpha(peekch(scr)))
		nextch(scr);

	emit(scr, TOK_VAR);
	return lexany(scr);
}

void*
lexdigit(scanner *scr)
{
	while (isdigit(peekch(scr)))
		nextch(scr);

	emit(scr, TOK_DIG);
	return lexany(scr);
}

void*
lexspace(scanner *scr)
{
	while (isspace(peekch(scr)))
		nextch(scr);

	ignore(scr);
	return lexany(scr);
}

void
freescr(scanner *scr)
{
	token *tok, *nxt;

	if (!scr)
		return;

	pthread_join(*scr->thread, NULL); /* TODO stop the thread instead. */
	if (pthread_mutex_destroy(scr->qmutex))
		die("pthread_mutex_destroy failed");

	free(scr->thread);
	TAILQ_FOREACH_SAFE(tok, &scr->qhead, toks, nxt) {
		if (tok != &septok) {
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
	scr->line   = 1;
	scr->start  = 0;
	scr->input  = estrdup(str);
	scr->inlen  = strlen(str);
	scr->qmutex = emalloc(sizeof(pthread_mutex_t));
	scr->thread = emalloc(sizeof(pthread_t));

	scr->fullsem  = emalloc(sizeof(sem_t));
	scr->emptysem = emalloc(sizeof(sem_t));

	/* TODO adjust initial emptysem value */
	if (sem_init(scr->fullsem, 0, 0)
			|| sem_init(scr->emptysem, 0, 250))
		die("sem_init failed");

	TAILQ_INIT(&scr->qhead);
	TAILQ_INSERT_TAIL(&scr->qhead, &septok, toks);

	if (pthread_mutex_init(scr->qmutex, NULL))
		die("pthread_mutex_init failed");
	if (pthread_create(scr->thread, NULL, lexany, (void*)scr))
		die("pthread_create failed");

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
