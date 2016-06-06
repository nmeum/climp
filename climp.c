#include <stdio.h>

#include <sys/queue.h>
#include <semaphore.h>
#include <pthread.h>

#include "scanner.h"

int
main(void)
{
	token *tok;
	scanner *scr;

	scr = scanstr("(  :=  )");
	for (tok = nxttok(scr); tok; tok = nxttok(scr))
		printf("TOKEN: %s\n", tok->text);

	freescr(scr);
	return 0;
}
