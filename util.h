#define die(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

char *estrdup(char *s);
void *emalloc(size_t size);
