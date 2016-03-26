#define die(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)
