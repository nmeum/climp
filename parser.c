#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <sys/queue.h>

#include "climp.h"
#include "parser.h"
#include "scanner.h"

statement*
newstmt(void)
{
	statement *stmt;

	if (!(stmt = malloc(sizeof(*stmt))))
		die("malloc failed");

	return stmt;
}

void
freestmt(statement *stmt)
{
	if (!stmt) return;

	if (stmt->type == STMT_ERROR)
		free(stmt->d.error.msg);

	free(stmt);
}

statement*
define(char *var, expr *exp)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_DEFINE;
	stmt->d.define.var = var;
	stmt->d.define.exp = exp;
	return stmt;
}

statement*
error(int line, char *msg, ...)
{
	int slen = 1;
	statement *stmt;
	va_list ap;
	char *dest;

	va_start(ap, msg);
	slen += vsnprintf(NULL, 0, msg, ap);
	if (!(dest = malloc(slen * sizeof(char*))))
		die("malloc failed");

	vsnprintf(dest, slen, msg, ap);
	va_end(ap);

	stmt = newstmt();
	stmt->type = STMT_ERROR;
	stmt->d.error.msg = dest;
	stmt->d.error.line = line;
	return stmt;
}

expr*
expression(token *tok)
{
	// TODO
	return NULL;
}

statement*
letstmt(scanner *scr)
{
	token *tok;
	char *var;
	expr *exp;

	tok = nxttok(scr);
	if (tok->type != TOK_VAR || strcmp(tok->text, "let"))
		return error(tok->line, "Expected '%s' got '%s'",
				"let", tok->text);
	
	tok = nxttok(scr);
	if (tok->type != TOK_VAR)
		return error(tok->line, "Expected variable after 'let'");
	else
		var = tok->text;

	tok = nxttok(scr);
	if (tok->type != TOK_ASSIGN)
		return error(tok->line, "Expected ':=', got '%s'", tok->text);

	if (!(exp = expression(nxttok(scr))))
		return error(tok->line, "Expected expression after ':='");

	return define(var, exp);
}
