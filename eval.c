#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/queue.h>

#include "env.h"
#include "scanner.h"
#include "parser.h"
#include "eval.h"

evalerr eval(env *e, statement **s);

evalerr
evalop(binop op, int arg1, int arg2, int *dest)
{
	switch (op) {
		case OP_PLUS:
			if (__builtin_sadd_overflow(arg1, arg2, dest))
				return ERR_PRECISION;
			break;
		case OP_MINUS:
			if (__builtin_ssub_overflow(arg1, arg2, dest))
				return ERR_PRECISION;
			break;
		case OP_MULTI:
			if (__builtin_smul_overflow(arg1, arg2, dest))
				return ERR_PRECISION;
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
execread(env *vars, statement *stmt)
{
	int input;

	if (isatty(0) && printf("Input: ") <= 0)
		return ERR_PRINTF;
	if (fflush(stdout))
		return ERR_FLUSH;

	if (scanf("%d", &input) == EOF)
		return ERR_SCANF;
	if (updval(vars, stmt->d.read.var, input))
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

	if (isatty(0)) {
		if (printf("Value: %d\n", value) <= 0)
			return ERR_PRINTF;
	} else {
		printf("%d\n", value);
	}

	return EVAL_OK;
}

evalerr
execcond(env *vars, statement *stmt)
{
	evalerr ret;
	int value;

	if ((ret = evaluate(vars, stmt->d.cond.cond, &value))
			!= EVAL_OK)
		return ret;

	if (value == 0)
		return eval(vars, stmt->d.cond.brn2);
	else
		return eval(vars, stmt->d.cond.brn1);
}

evalerr
execloop(env *vars, statement *stmt)
{
	evalerr ret;
	int value;

	if ((ret = evaluate(vars, stmt->d.loop.cond, &value))
			!= EVAL_OK)
		return ret;

	while (value != 0) {
		if ((ret = eval(vars, stmt->d.loop.brn)) != EVAL_OK)
			return ret;

		if ((ret = evaluate(vars, stmt->d.loop.cond, &value))
				!= EVAL_OK)
			return ret;
	}

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
		case STMT_READ:
			return execread(vars, stmt);
		case STMT_WRITE:
			return execwrite(vars, stmt);
		case STMT_COND:
			return execcond(vars, stmt);
		case STMT_LOOP:
			return execloop(vars, stmt);
	}

	/* Never reached. */
	return EVAL_OK;
}

evalerr
eval(env *vars, statement **cmds)
{
	evalerr ret;
	statement *stmt;

	while ((stmt = *cmds++))
		if ((ret = execute(vars, stmt)) != EVAL_OK)
			return ret;

	return EVAL_OK;
}
