#include <stdio.h>

#include <sys/queue.h>
#include "scanner.h"

int
main(void)
{
	token *tok;
	scanner *scr;

	scr = scanstr("123");
	for (tok = nxttok(scr); tok; tok = nxttok(scr))
		printf("%s", tok->text);

	freescr(scr);
	return 0;
}
