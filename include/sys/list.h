#pragma once

#include "arm/critical.h"

typedef struct list_node_s list_node_s;

#define list_head_s list_node_s

typedef struct list_node_s {
	list_node_s *prev, *next;
} list_node_s;

#define list_next(n)	((n)->next)
#define list_prev(n)	((n)->prev)

#define list_head(h)	list_next(h)
#define list_tail(h)	list_prev(h)

#define list_node_init(n)	do{(n)->prev = (n)->next = (n);}while(0)
#define list_head_init(h)	list_node_init(h)

#define list_node_embedded(n)	((n) != ((n)->prev))

#define list_empty(h)	(!list_node_embedded(h))
#define list_add(n, h)	list_add_tail(n, h)

static inline void list_remove(list_node_s *n) {
	n->next->prev = n->prev;
	n->prev->next = n->next;
	n->prev = n->next = n;
}

static inline void list_add_head(list_node_s *n, list_head_s *h) {
	n->next = h->next;
	h->next->prev = n;
	n->prev = h;
	h->next = n;
}

static inline void list_add_tail(list_node_s *n, list_head_s *h) {
	n->prev = h->prev;
	h->prev->next = n;
	n->next = h;
	h->prev = n;
}

static inline bool list_append_if_not_embedded(list_node_s *n, list_head_s *h) {
	need_critical();

	if (!list_node_embedded(n)) {
		list_add_tail(n, h);
		return true;
	}
	return false;
}
