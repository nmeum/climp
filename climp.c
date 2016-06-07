#include <stdio.h>

#include <sys/queue.h>
#include <semaphore.h>
#include <pthread.h>

#include "scanner.h"
#include "parser.h"

int
main(void)
{
	parser par;
	scanner *scr;
	statement *st;

	scr = scanstr("let foo := 1");
	par.scr = scr;
	par.peektok = NULL;

	st = stmt(&par);
	printf("%d\n", st->type);

	freescr(scr);
	return 0;
}
