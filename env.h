typedef struct entry entry;

struct entry {
	char *key;
	int value;
	entry *next;
};

typedef struct env env;

struct env {
	int size;
	entry **entries;
};

env *newenv(int s);
void freeenv(env *e);
int setval (env *e, char *k, int v);
int getval (env *e, char *k, int *d);
int updval (env *e, char *k, int v);
