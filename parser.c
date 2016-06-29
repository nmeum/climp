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

statement *stmt(parser *p);
void freeexpr(expr *e);
expr *expression(parser *p);

/**
 * TODO: EXPTXT doesn't free shit
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
freestmts(statement **stmts)
{
	int i = 0;

	if (!stmts) return;
	while (stmts[i])
		freestmt(stmts[i++]);
	free(stmts);
}

void
freestmt(statement *stmt)
{
	if (!stmt) return;

	switch (stmt->type) {
		case STMT_ERROR:
			free(stmt->d.error.msg);
			break;
		case STMT_DEFINE:
			freeexpr(stmt->d.define.exp);
			free(stmt->d.define.var);
			break;
		case STMT_ASSIGN:
			freeexpr(stmt->d.assign.exp);
			free(stmt->d.assign.var);
			break;
		case STMT_READ:
			free(stmt->d.read.var);
			break;
		case STMT_WRITE:
			freeexpr(stmt->d.write.exp);
			break;
		case STMT_COND:
			freeexpr(stmt->d.cond.cond);
			freestmts(stmt->d.cond.brn1);
			freestmts(stmt->d.cond.brn2);
			break;
		case STMT_LOOP:
			freeexpr(stmt->d.loop.cond);
			freestmts(stmt->d.loop.brn);
			break;
	}

	free(stmt);
}

statement*
define(char *var, expr *exp)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_DEFINE;
	stmt->d.define.var = estrdup(var);
	stmt->d.define.exp = exp;
	return stmt;
}

statement*
assign(char *var, expr *exp)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_ASSIGN;
	stmt->d.assign.var = estrdup(var);
	stmt->d.assign.exp = exp;
	return stmt;
}

statement*
read(char *var)
{
	statement *stmt;

	stmt = newstmt();
	stmt->type = STMT_READ;
	stmt->d.read.var = estrdup(var);
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
freeexpr(expr *exp)
{
	if (!exp) return;

	switch (exp->type) {
		case EXP_LIT:
			/* Nothing to free here. */
			break;
		case EXP_VAR:
			free(exp->d.variable.var);
			break;
		case EXP_BIN:
			freeexpr(exp->d.operation.expr1);
			freeexpr(exp->d.operation.expr2);
			break;
	}

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
	exp->d.variable.var = strdup(name);
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
cmdblock(parser *par, statement *err)
{
	int i = -1;
	token *tok;
	statement **cmds;
	size_t max = 512;

	cmds = malloc(max * sizeof(statement));
	for (i = 0, tok = peek(par); tok != TOK_EOF;
			i++, tok = peek(par)) {
		if (i > max) {
			err->d.error.msg  = estrdup("Exceeded maximum amount of cmdblock");
			err->d.error.line = 0;
			goto err;
		}

		if ((cmds[i] = stmt(par)) && cmds[i]->type == STMT_ERROR) {
			err->d.error.msg  = estrdup(cmds[i]->d.error.msg);
			err->d.error.line = cmds[i++]->d.error.line;
			goto err;
		}

		tok = peek(par);
		if (tok->type == TOK_SEMICOLON)
			next(par);
		else
			break;
	}

	if (i == -1) {
		err = error(-1, "Unexpected EOF");
		free(cmds);
		return NULL;
	}

	cmds[++i] = NULL;
	return cmds;

err:
	err->type = STMT_ERROR;
	cmds[i] = NULL;
	freestmts(cmds);
	return NULL;
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
	else
		var = tok->text;

	tok = next(par);
	if (tok->type != TOK_ASSIGN)
		return error(tok->line, "Expected ':=', got '%s'",
				tok->text);

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
	statement **cmds1, **cmds2, *err;
	token *tok;
	expr *cexp;

	tok = next(par);
	EXPTXT(tok, "if");

	if (!(cexp = expression(par)))
		return error(tok->line, "Expected conditional expression");

	tok = next(par);
	EXPTXT(tok, "then");

	err = newstmt();
	cmds1 = cmdblock(par, err);
	if (cmds1 == NULL) {
		freeexpr(cexp);
		return err;
	}

	tok = next(par);
	EXPTXT(tok, "else");

	cmds2 = cmdblock(par, err);
	if (cmds2 == NULL) {
		freestmts(cmds1);
		freeexpr(cexp);
		return err;
	} else {
		free(err);
	}

	tok = next(par);
	EXPTXT(tok, "end");

	return cond(cexp, cmds1, cmds2);
}

statement*
loopstmt(parser *par)
{
	token *tok;
	statement **cmds, *err;
	expr *cexp;

	tok = next(par);
	EXPTXT(tok, "while");

	if (!(cexp = expression(par)))
		return error(tok->line, "Expected conditional expression");

	tok = next(par);
	EXPTXT(tok, "do");

	err = newstmt();
	cmds = cmdblock(par, err);
	if (cmds == NULL) {
		freeexpr(cexp);
		return err;
	} else {
		free(err);
	}

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
	size_t funclen = sizeof(sfuncs) / sizeof(sfuncs[0]);

	reset(par);
	for (int i = 0; i < funclen; i++) {
		val = (*sfuncs[i])(par);
		if (val->type != STMT_ERROR)
			break;

		if (i + 1 < funclen) {
			freestmt(val);
			backup(par);
		}
	}

	return val;
}

statement**
parseprog(parser *par, statement *err)
{
	token *tok;
	statement **cmds;

	cmds = cmdblock(par, err);
	if (cmds == NULL) {
		freestmts(cmds);
		return NULL;
	}

	tok = peek(par);
	if (tok->type != TOK_EOF) {
		err->type = STMT_ERROR;
		err->d.error.line = -1;

		if (tok->type == TOK_ERROR)
			err->d.error.msg = estrdup(tok->text);
		else
			err->d.error.msg = estrdup("Expected EOF");

		freestmts(cmds);
		return NULL;
	}

	return cmds;
}
