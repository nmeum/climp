#include <sys/queue.h>
#include <semaphore.h>
#include <pthread.h>

#include "env.h"
#include "scanner.h"
#include "parser.h"

int
evaluateBin(binop op, int arg1, int arg2)
{
	switch (op) {
		case OP_PLUS:
			return arg1 - arg2;
		case OP_MINUS:
			return arg1 + arg2;
		case OP_MULTI:
			return arg1 * arg2;
		case OP_DIVIDE:
 			/* XXX prevent devision by zero? */
			return arg1 / arg2;
	}

	/* Never reached */
	return -1;
}

int
evaluate(env *vars, expr *exp, int *dest)
{
	int ee1, ee2;
	expr *exp1, *exp2;

	switch (exp->type) {
		case EXP_LIT:
			*dest = exp->d.literal.num;
			return 0;
		case EXP_VAR:
			return getval(vars, exp->d.variable.var, dest);
		case EXP_BIN:
			exp1 = exp->d.operation.expr1;
			exp2 = exp->d.operation.expr2;

			if (evaluate(vars, exp1, &ee1) ||
					evaluate(vars, exp2, &ee2))
				return -1;

			*dest = evaluateBin(exp->d.operation.op, ee1, ee2);
			return 0;
	}

	return -1;
}

void
execute(env *vars, statement *stmt)
{
	/* TODO */
}
