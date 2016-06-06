#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/queue.h>

#include "util.h"
#include "scanner.h"
#include "parser.h"

expr *expression(parser *par);

/**
 * TODO: strdup for char pointers in statements / expressions.
 * TODO: better error handling when parsing expressions.
 */

token*
next(parser *par)
{
	token *tok;

	tok = par->peektok;
	if (tok != NULL)
		par->peektok = NULL;
	else
		tok = nxttok(par->scr);

	return tok;
}

token*
peek(parser *par)
{
	token *tok;

	tok = par->peektok;
	if (tok != NULL)
		return tok;

	par->peektok = nxttok(par->scr);
	return par->peektok;
}

statement*
newstmt(void)
{
	return emalloc(sizeof(statement));
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
	dest = emalloc(slen * sizeof(char*));

	vsnprintf(dest, slen, msg, ap);
	va_end(ap);

	stmt = newstmt();
	stmt->type = STMT_ERROR;
	stmt->d.error.msg = dest;
	stmt->d.error.line = line;
	return stmt;
}

expr*
newexpr(void)
{
	return emalloc(sizeof(expr));
}

void
freexpr(expr *exp)
{
	if (!exp) return;
	free(exp);
}

expr*
literal(int num)
{
	expr *exp;

	exp = newexpr();
	exp->type = EXP_LIT;
	exp->d.literal.num = num;
	return exp;
}

expr*
variable(char *name)
{
	expr *exp;

	exp = newexpr();
	exp->type = EXP_VAR;
	exp->d.variable.var = name;
	return exp;
}

expr*
factor(parser *par)
{
	expr *exp;
	token *tok;

	tok = next(par);
	if (tok->type == TOK_DIG) {
		return literal(atoi(tok->text));
	} else if (tok->type == TOK_VAR) {
		return variable(tok->text);
	}

	tok = next(par);
	if (tok->type == TOK_LBRACKET) {
		if (!(exp = expression(par)))
			return NULL;

		tok = next(par);
		if (tok->type != TOK_RBRACKET)
			return NULL;
	}

	return NULL;
}

expr*
term(parser *par)
{
	binop op;
	expr *fac;
	token *tok;

	if (!(fac = factor(par)))
		return NULL;

	tok = next(par);
	switch (tok->type) {
		case TOK_MULTI:
			op = OP_MULTI;
			break;
		case TOK_DIVIDE:
			op = OP_DIVIDE;
			break;
		default:
			return fac;
	}

	/* TODO: figure out if next elem is a factor.
	 * requires some kind of backup or peektok func. */
	(void)op;
	return NULL;
}

expr*
expression(parser *par)
{
	/* TODO */
	return NULL;
}

statement*
letstmt(parser *par)
{
	token *tok;
	char *var;
	expr *exp;

	tok = next(par);
	if (tok->type != TOK_VAR || strcmp(tok->text, "let"))
		return error(tok->line, "Expected '%s' got '%s'",
				"let", tok->text);
	
	tok = next(par);
	if (tok->type != TOK_VAR)
		return error(tok->line, "Expected variable after 'let'");
	else
		var = tok->text;

	tok = next(par);
	if (tok->type != TOK_ASSIGN)
		return error(tok->line, "Expected ':=', got '%s'", tok->text);

	if (!(exp = expression(par)))
		return error(tok->line, "Expected expression after ':='");

	return define(var, exp);
}
