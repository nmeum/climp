#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

#include <sys/queue.h>

#include "env.h"
#include "scanner.h"
#include "parser.h"
#include "eval.h"

evalerr
evalop(binop op, int arg1, int arg2, int *dest)
{
	switch (op) {
		case OP_PLUS:
			*dest = arg1 + arg2;
			break;
		case OP_MINUS:
			*dest = arg1 - arg2;
			break;
		case OP_MULTI:
			*dest = arg1 * arg2;
			break;
		case OP_DIVIDE:
			if (arg2 == 0)
				return ERR_DIVBYZERO;

			*dest = arg1 / arg2;
			break;
	}

	return EVAL_OK;
}

evalerr
evaluate(env *vars, expr *exp, int *dest)
{
	evalerr ret;
	int ee1, ee2;
	expr *exp1, *exp2;

	switch (exp->type) {
		case EXP_LIT:
			*dest = exp->d.literal.num;
			break;
		case EXP_VAR:
			if (getval(vars, exp->d.variable.var, dest))
				return ERR_UNDEFVAR;
			else
				break;
		case EXP_BIN:
			exp1 = exp->d.operation.expr1;
			exp2 = exp->d.operation.expr2;

			if ((ret = evaluate(vars, exp1, &ee1)) != EVAL_OK ||
					(ret = evaluate(vars, exp2, &ee2)) != EVAL_OK)
				return ret;

			if ((ret = evalop(exp->d.operation.op, ee1, ee2, dest))
					!= EVAL_OK)
				return ret;

			break;
	}

	return EVAL_OK;
}

evalerr
execdefine(env *vars, statement *stmt)
{
	evalerr ret;
	int value;

	if ((ret = evaluate(vars, stmt->d.define.exp, &value))
			!= EVAL_OK)
		return ret;

	if (setval(vars, stmt->d.define.var, value))
		return ERR_VARDEFINED;

	return EVAL_OK;
}

evalerr
execassign(env *vars, statement *stmt)
{
	evalerr ret;
	int value;

	if ((ret = evaluate(vars, stmt->d.assign.exp, &value))
			!= EVAL_OK)
		return ret;

	if (updval(vars, stmt->d.assign.var, value))
		return ERR_UNINITVAR;

	return EVAL_OK;
}

evalerr
execwrite(env *vars, statement *stmt)
{
	evalerr ret;
	int value;

	if ((ret = evaluate(vars, stmt->d.write.exp, &value))
			!= EVAL_OK)
		return ret;

	if (printf("Value: %d\n", value) <= 0)
		return ERR_PRINTF;

	return EVAL_OK;
}

evalerr
execute(env *vars, statement *stmt)
{
	switch (stmt->type) {
		case STMT_ERROR:
			break; /* Never reached */
		case STMT_DEFINE:
			return execdefine(vars, stmt);
		case STMT_ASSIGN:
			return execassign(vars, stmt);
		case STMT_WRITE:
			return execwrite(vars, stmt);
		default:
			/* TODO */
			break;
	}

	/* Never reached. */
	return EVAL_OK;
}

evalerr
eval(statement **cmds)
{
	env *vars;
	evalerr ret;
	statement *stmt;

	vars = newenv(512);
	while ((stmt = *cmds++))
		if ((ret = execute(vars, stmt)) != EVAL_OK)
			return ret;

	freeenv(vars);
	return EVAL_OK;
}
