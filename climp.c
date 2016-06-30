#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include "env.h"
#include "scanner.h"
#include "parser.h"
#include "eval.h"
#include "util.h"

enum {
	VARTBLSIZ = 512,
};

int
evalstr(char *input)
{
	int r = 1;
	env *vars;
	evalerr ret;
	statement **cmds, *err = emalloc(sizeof(statement));
	parser *par;

	par = newpar(input);
	if (!(cmds = parseprog(par, err))) {
		if (err->d.error.line >= 0)
			fprintf(stderr, "Parser error in line %d: %s\n",
					err->d.error.line, err->d.error.msg);
		else
			fprintf(stderr, "Parser error: %s\n", err->d.error.msg);

		freestmt(err);
		goto err;
	} else {
		free(err);
	}

	vars = newenv(VARTBLSIZ);
	if ((ret = eval(vars, cmds)) != EVAL_OK) {
		fprintf(stderr, "Runtime error: %d\n", ret);
		goto err;
	}

	r = 0;
	freeenv(vars);

err:
	freestmts(cmds);
	freepar(par);
	return r;
}

int
main(int argc, char **argv)
{
	int ret;
	FILE *fd;
	char *input, *fp = argv[1];
	struct stat st;

	if (argc <= 1)
		return 1;

	if (stat(fp, &st))
		die("stat failed");
	else
		input = emalloc(st.st_size + 1);

	if (!(fd = fopen(fp, "r")))
		die("fopen failed");
	if (fread(input, sizeof(char), st.st_size, fd) != st.st_size)
		die("short read");

	if (fclose(fd))
		die("fclose failed");

	input[st.st_size] = '\0';
	ret = evalstr(input);

	free(input);
	return ret;
}
