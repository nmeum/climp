#include <stdio.h>

#include <sys/queue.h>
#include "scanner.h"

int
main(void)
{
	token *tok;
	scanner *scr;

	scr = scanstr(";\n");
	for (tok = nxttok(scr); tok; tok = nxttok(scr))
		printf("%s\n", tok->text);

	freescr(scr);
	return 0;
}
