#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H

#include "entry.h"
#include "list.h"


/*
*	struct node_t
*/
struct node_t{
	struct entry_t *value;
	struct node_t *next;
};

/*
*	struct list_t
*/
struct list_t{
	struct node_t *head;
	int size;
};


/*
* compare method for qsort called at sortList
*/
int compare(const void *l1 , const void *l2);

/*
* sortList list
*/
int sortList(struct list_t *list, struct node_t *newNode);

/*
* insertion sort algorithm
*/
void pSort(struct node_t *current);

/*
* method to destroy and free memory
*/
void node_destroy(struct node_t *toDelete);



/*
*	node create
*/
struct node_t *node_create(struct entry_t *value);



#endif
