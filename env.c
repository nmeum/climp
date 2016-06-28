#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "util.h"

env*
newenv(int size)
{
	env *tbl;

	tbl = emalloc(sizeof(env));
	tbl->size = size;
	tbl->entries = emalloc(sizeof(entry) * size);

	for (int i = 0; i < size; i++)
		tbl->entries[i] = NULL;

	return tbl;
}

void
freeentry(entry *ent)
{
	entry *next;

	if (!ent) return;
	free(ent->key);

	for (next = ent->next; next != NULL; next = next->next)
		freeentry(next);

	free(ent);
}

void
freeenv(env *tbl)
{
	if (!tbl) return;
	for (int i = 0; i < tbl->size; i++)
		freeentry(tbl->entries[i]);

	free(tbl->entries);
	free(tbl);
}

int
hash(env *tbl, char *str)
{
	int c;
	unsigned long hash = 5381;

	/**
	 * Borrowed from DJB http://www.cse.yorku.ca/~oz/hash.html
	 */

	while ((c = *str++))
		 /* hash * 33 + c */
		hash = ((hash << 5) + hash) + c;

	return hash % tbl->size;
}

void
setval(env *tbl, char *key, int val)
{
	int keyh;
	entry *ent, *buck, *last, *next;

	ent = emalloc(sizeof(entry));
	ent->key   = estrdup(key);
	ent->value = val;
	ent->next  = NULL;

	keyh = hash(tbl, key);
	if (!(buck = tbl->entries[keyh])) {
		tbl->entries[keyh] = ent;
		return;
	}

	for (next = buck; next != NULL; next = next->next) {
		if (!strcmp(next->key, key))
			return;
		last = next;
	}

	last->next = ent;
}

int
getval(env *tbl, char *key, int *dest)
{
	int keyh;
	entry *buck, *next;

	keyh = hash(tbl, key);
	if (!(buck = tbl->entries[keyh]))
		return -1;

	for (next = buck; next != NULL; next = next->next)
		if (!strcmp(next->key, key))
			break;

	*dest = next->value;
	return 0;
}
