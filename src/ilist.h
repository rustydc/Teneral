#ifndef __ILIST_H__
#define __ILIST_H__

// Each item should start with an int?
typedef struct ilist_s {
	int **items;
	int len;
	int size;
} ilist_t;

ilist_t *new_ilist(int size);
void ilist_insert(ilist_t *list, void *item);
void *ilist_fetch(ilist_t *list, int id);
void *ilist_remove(ilist_t *list, int id);
void *ilist_get(ilist_t *list, int n);

int bin_search(ilist_t *list, int id, int *pos);

#endif
