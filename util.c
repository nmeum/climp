#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char*
estrdup(char *s) {
	char *r;

	if (!(r = strdup(s)))
		die("strdup failed");

	return r;
}

void*
emalloc(size_t size)
{
	void *r;

	if (!(r = malloc(size)))
		die("malloc failed");

	return r;
}
