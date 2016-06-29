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

	par = newpar("let x := 2; x := 3; ! x");
	/* par = newpar("while foo do error end"); */

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
