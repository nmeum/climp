#include <stdio.h>

#include <sys/queue.h>
#include <semaphore.h>
#include <pthread.h>

#include "scanner.h"
#include "parser.h"

int
main(void)
{
	statement *st;
	parser *par;

	par = newpar("if 1 then let foo := 1 else bar := 5 end");

	st = stmt(par);
	if (st->type == STMT_ERROR) {
		printf("ERROR: %s\n", st->d.error.msg);
	} else {
		printf("TYPE: %d\n", st->type);
	}

	freepar(par);
	return 0;
}
