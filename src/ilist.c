#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ilist.h"

// Create a ilist with 'size' capacity.
ilist_t *new_ilist(int size)
{
	ilist_t *list = malloc(sizeof(ilist_t));
	list->items = malloc(sizeof(void *) * size);
	list->len = 0;
	list->size = size;

	return list;
}

void printlist(ilist_t *list) {
	int i;
	for (i = 0; i != list->len; i++) {
		printf("list[%d]: %d\n", i, *((int*)list->items[i]));
	}
}

void ilist_insert(ilist_t *list, void *item)
{
	int pos;
	int id = *(int*)item;

	if (list->len == 0) {
		pos = 0;
	} else {

		int loc = bin_search(list, id, &pos);
	
		if (loc != -1) {
			printf("ilist: Dup insert ignored.\n");
			return;
		}

		bcopy(list->items+pos, list->items+pos+1, ((list->len)-pos) * sizeof(int*));
	}
	list->items[pos] = item;
	
	list->len++;
}

// Return the location if we found it exactly, or -1.
// Set 'pos' to the position to insert at if it isn't there
int bin_search(ilist_t *list, int id, int *pos) {
	int min = 0;
	int max = list->len-1;
	int mid, diff;

	// List length zero:  not found, insert at 0.
	if (list->len == 0) {
		*pos = 0;
		return -1;
	}

	while (min <= max) {
		mid = (min+max)/2;
		diff = *(list->items[mid]) - id;

		if (diff < 0) {
			min = mid + 1;
		} else if (diff > 0) {
			max = mid - 1;
		} else {
			// Found it.
			*pos = mid;
			return mid;
		}
	}

	// Was the last one we checked above or below?
	//  This determines whether we insert here or after it.
	if (diff > 0) {
		*pos = mid;
	} else {
		*pos = mid+1;
	}
	return -1;

}


void *ilist_fetch(ilist_t *list, int id) {
	int pos;
	int loc = bin_search(list, id, &pos);
	if (loc == -1) {
		return NULL;
	}

	return list->items[pos];
}

void *ilist_remove(ilist_t *list, int id) {
	int pos;
	int loc = bin_search(list, id, &pos);

	if (loc == -1) {
		printf("ilist: Tried to remove nonexistant: %d.\n", id);
		return NULL;
	}

	void *retval = list->items[pos];

	bcopy(list->items+pos+1, list->items+pos, list->len-(pos));
	list->len--;

	return retval;
	
}


void *ilist_get(ilist_t *list, int n) {
	if (n >= list->len) {
		printf("ilist: Get beyond list length.\n");
		return NULL;
	}

	return (list->items[n]);

}
