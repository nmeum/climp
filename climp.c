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

	par = newpar("foo := 1");

	st = stmt(par);
	if (st->type == STMT_ERROR) {
		printf("ERROR: %s\n", st->d.error.msg);
	} else {
		printf("TYPE: %d\n", st->type);
	}

	freepar(par);
	return 0;
}
