#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <sys/queue.h>

#include "climp.h"
#include "parser.h"
#include "scanner.h"

command*
newcmd(void)
{
	command *cmd;

	if (!(cmd = malloc(sizeof(*cmd))))
		die("malloc failed");

	return cmd;
}

void
freecmd(command *cmd)
{
	if (!cmd) return;

	if (cmd->type == CMD_ERROR)
		free(cmd->d.error.msg);

	free(cmd);
}

command*
define(char *var, expr *exp)
{
	command *cmd;

	cmd = newcmd();
	cmd->type = CMD_DEFINE;
	cmd->d.define.var = var;
	cmd->d.define.exp = exp;
	return cmd;
}

command*
error(int line, char *msg, ...)
{
	int slen = 1;
	command *cmd;
	va_list ap;
	char *dest;

	va_start(ap, msg);
	slen += vsnprintf(NULL, 0, msg, ap);
	if (!(dest = malloc(slen * sizeof(char*))))
		die("malloc failed");

	vsnprintf(dest, slen, msg, ap);
	va_end(ap);

	cmd = newcmd();
	cmd->type = CMD_ERROR;
	cmd->d.error.msg = dest;
	cmd->d.error.line = line;
	return cmd;
}

expr*
expression(token *tok)
{
	// TODO
	return NULL;
}

command*
letcmd(scanner *scr)
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
