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

#define EXPTXT(T, M) \
	do { if (T->type != TOK_VAR || strcmp(T->text, M)) \
		return error(T->line, "Expected '%s' got '%s'", M, T->text); \
	   } while (0)

expr *expression(parser *par);

/**
 * TODO: strdup for char pointers in statements / expressions.
 * TODO: better error handling when parsing expressions.
 * TODO: fix memory leaks
 */

parser*
newpar(char *str)
{
	parser *par;

	par = emalloc(sizeof(parser));
	par->scr = scanstr(str);
	par->max = 5 * 1024;
	par->cur = 0;
	par->buf = emalloc(par->max * sizeof(token));
	par->peektok = NULL;

	for (int i = 0; i <= par->max; i++)
		par->buf[i] = NULL;

	return par;
}

void
freepar(parser *par)
{
	freescr(par->scr);
	if (par->peektok)
		free(par->peektok);

	free(par->buf);
	free(par);
}

void
reset(parser *par)
{
	for (int i = 0; i <= par->cur; i++)
		par->buf[i] = NULL;

	par->cur = 0;
	if (par->peektok != NULL)
		par->buf[0] = par->peektok;
}

void
backup(parser *par)
{
	par->cur = 0;
}

token*
peek(parser *par)
{
	token *tok;

	if (par->cur > par->max)
		reset(par); /* FIXME */

	tok = par->buf[par->cur];
	if (tok == NULL) {
		tok = nxttok(par->scr);
		par->buf[par->cur] = tok;
		par->peektok = tok;
	} else {
		par->peektok = NULL;
	}

	return tok;
}

token*
next(parser *par)
{
	token *tok;

	tok = peek(par);
	par->cur++;

	return tok;
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
assign(char *var, expr *exp)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_ASSIGN;
	stmt->d.assign.var = var;
	stmt->d.assign.exp = exp;
	return stmt;
}

statement*
read(char *var)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_READ;
	stmt->d.read.var = var;
	return stmt;
}

statement*
write(expr *exp)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_WRITE;
	stmt->d.write.exp = exp;
	return stmt;
}

statement*
cond(expr *cexp, statement **brn1, statement **brn2)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_COND;
	stmt->d.cond.cond = cexp;
	stmt->d.cond.brn1 = brn1;
	stmt->d.cond.brn2 = brn2;
	return stmt;
}

statement*
loop(expr *cexp, statement **brn)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_LOOP;
	stmt->d.loop.cond = cexp;
	stmt->d.loop.brn  = brn;
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
operation(binop op, expr *expr1, expr *expr2)
{
	expr *exp;

	exp = newexpr();
	exp->type = EXP_BIN;
	exp->d.operation.op = op;
	exp->d.operation.expr1 = expr1;
	exp->d.operation.expr2 = expr2;
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

		return exp;
	}

	return NULL;
}

expr*
term(parser *par)
{
	binop op;
	expr *fac1, *fac2;
	token *tok;

	if (!(fac1 = factor(par)))
		return NULL;

	tok = peek(par);
	switch (tok->type) {
		case TOK_MULTI:
			op = OP_MULTI;
			next(par);
			break;
		case TOK_DIVIDE:
			op = OP_DIVIDE;
			next(par);
			break;
		default:
			return fac1;
	}

	if (!(fac2 = factor(par)))
		return NULL;

	return operation(op, fac1, fac2);
}

expr*
expression(parser *par)
{
	binop op;
	expr *term1, *term2;
	token *tok;

	if (!(term1 = term(par)))
		return NULL;

	tok = peek(par);
	switch (tok->type) {
		case TOK_PLUS:
			op = OP_PLUS;
			next(par);
			break;
		case TOK_MINUS:
			op = OP_MINUS;
			next(par);
			break;
		default:
			return term1;
	}

	if (!(term2 = term(par)))
		return NULL;

	return operation(op, term1, term2);
}

statement**
commands(parser *par)
{
	size_t i = -1;
	token *tok;
	statement **cmds;
	size_t max = 1024;

	cmds = malloc(max * sizeof(statement));
	for (i = 0, tok = peek(par); tok != TOK_EOF;
			i++, tok = peek(par)) {
		if (i >= max) break; /* FIXME */
		cmds[i] = stmt(par);

		tok = peek(par);
		if (tok->type == TOK_SEMICOLON)
			next(par);
		else
			break;
	}

	if (i == -1)
		return NULL;
	else
		cmds[++i] = NULL;

	return cmds;
}

statement*
letstmt(parser *par)
{
	token *tok;
	char *var;
	expr *exp;

	tok = next(par);
	EXPTXT(tok, "let");
	
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

statement*
assignstmt(parser *par)
{
	token *tok;
	expr *val;
	char *var;

	tok = next(par);
	if (tok->type != TOK_VAR)
		return error(tok->line, "Expected variable for assigment");

	tok = next(par);
	if (tok->type != TOK_ASSIGN)
		return error(tok->line, "Expected ':=', got '%s'",
				tok->text);
	else
		var = tok->text;

	if (!(val = expression(par)))
		return error(tok->line, "Expected expression after ':='");

	return assign(var, val);
}

statement*
readstmt(parser *par)
{
	token *tok;

	tok = next(par);
	if (tok->type != TOK_QUESTION)
		return error(tok->line, "Expected '?' got '%s'",
				tok->text);

	tok = next(par);
	if (tok->type != TOK_VAR)
		return error(tok->line, "Expected variable after operator");

	return read(tok->text);
}

statement*
writestmt(parser *par)
{
	token *tok;
	expr *val;

	tok = next(par);
	if (tok->type != TOK_EXCLAMATION)
		return error(tok->line, "Expected '!' got '%s'",
				tok->text);

	if (!(val = expression(par)))
		return error(tok->line, "Expected expression after operator");

	return write(val);
}

statement*
condstmt(parser *par)
{
	statement **cmds1, **cmds2;
	token *tok;
	expr *cexp;

	tok = next(par);
	EXPTXT(tok, "if");

	if (!(cexp = expression(par)))
		return error(tok->line, "Expected conditional expression");

	tok = next(par);
	EXPTXT(tok, "then");

	cmds1 = commands(par);
	tok = next(par);
	EXPTXT(tok, "else");

	cmds2 = commands(par);
	tok = next(par);
	EXPTXT(tok, "end");

	return cond(cexp, cmds1, cmds2);
}

statement*
loopstmt(parser *par)
{
	token *tok;
	statement **cmds;
	expr *cexp;

	tok = next(par);
	EXPTXT(tok, "while");

	if (!(cexp = expression(par)))
		return error(tok->line, "Expected conditional expression");

	tok = next(par);
	EXPTXT(tok, "do");

	cmds = commands(par);
	tok = next(par);
	EXPTXT(tok, "end");

	return loop(cexp, cmds);
}

statement*
stmt(parser *par)
{
	statement *val;
	statement *(*sfuncs[])(parser *p) = {
		letstmt,
		assignstmt,
		readstmt,
		writestmt,
		condstmt,
		loopstmt,
	};

	reset(par);
	for (int i = 0; i < sizeof(sfuncs) / sizeof(sfuncs[0]); i++) {
		val = (*sfuncs[i])(par);

		if (val->type != STMT_ERROR)
			break;
		else
			backup(par);
	}

	return val;
}
