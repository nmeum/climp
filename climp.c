#include <stdio.h>
#include <stdlib.h>

#include <sys/queue.h>
#include <semaphore.h>
#include <pthread.h>

#include "env.h"
#include "scanner.h"
#include "parser.h"
#include "eval.h"
#include "util.h"

int
main(void)
{
	evalerr ret;
	statement **cmds, *err = emalloc(sizeof(statement));
	parser *par;

	/* par = newpar("let x := 0; if x then x := 10 else x := 20 end; ! x"); */
	par = newpar("let x := 5; while x do x := x - 1 end; ! x");

	cmds = commands(par, err);
	if (cmds == NULL) {
		printf("Parser error: %s\n", err->d.error.msg);
		freestmt(err);
	} else {
		free(err);
	}

	if (cmds && (ret = eval(cmds)) != EVAL_OK) {
		printf("Runtime error: %d\n", ret);
	}

	freestmts(cmds);
	freepar(par);
	return 0;
}
