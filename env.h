typedef struct entry entry;

struct entry {
	char *key, *value;
	entry *next;
};

typedef struct env env;

struct env {
	int size;
	entry **entries;
};

env *newenv(int s);
void freeenv(env *e);
void setval (env *e, char *k, char *v);
char *getval (env *e, char *k);
