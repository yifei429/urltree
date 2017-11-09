/*
 *
 *
 *
 *
 *
 **/


#ifndef __H_PROXY_TIMER_H__
#define __H_PROXY_TIMER_H__

#include <string.h>
#include <pthread.h>
#include "rbtree.h"

#ifndef offset_of
#define offset_of(type, member) ((size_t) &((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                 \
({                                                      \
        char *__mptr = (char *)(ptr);                   \
	if (ptr) __mptr -= offset_of(type, member);     \
	(type *)__mptr;                                 \
})
#endif


typedef int (*ptimer_out_handler)(void *arg);
typedef struct _ptimer_node_t {
	struct rb_node tm_node;
	ptimer_out_handler handler;
	
	u_int64_t clock;
	void *arg;
} ptimer_node_t;

typedef struct _ptimer_tree_t {
	struct rb_root root;
	int     count;
	pthread_rwlock_t lock; 
	int	lock_enabled;
} ptimer_tree_t;

static inline int ptimer_compare(ptimer_node_t *node1, ptimer_node_t *node2)
{
	if(node1->clock > node2->clock) {
		return 1;
	} else if(node1->clock == node2->clock){
		return 0; 
	} else {
		return -1;
	}
}

static inline int ptimer_init(ptimer_tree_t *tree, int locked)
{
	if (unlikely(!tree))
		return -1;

	tree->root = RB_ROOT;
	tree->count = 0;
	tree->lock_enabled = locked;
	if (locked) { 
		if(pthread_rwlock_init(&tree->lock, NULL)) {
			return -1;
		}
	}
	return 0;
}

static inline int ptimer_add(ptimer_tree_t *tree, ptimer_node_t *ptimer, ptimer_out_handler handler, int timeout, void *arg)
{


	struct rb_node **n = &(tree->root.rb_node), *p = NULL;
	int res = 0;
	ptimer_node_t *ptimer1;

	if(unlikely(!tree || !ptimer || !handler || !arg))
		return -1;

	if (tree->lock_enabled)
		pthread_rwlock_wrlock(&tree->lock);

	memset(ptimer, 0x0, sizeof(ptimer_node_t));
	ptimer->clock = time(NULL) + timeout;
	ptimer->handler = handler;
	ptimer->arg = arg;

	while(*n) {
		p = *n;
		ptimer1 = rb_entry(p, ptimer_node_t, tm_node); 
		res = ptimer_compare(ptimer, ptimer1);
		
		if (res < 0) 
			n = &((*n)->rb_left);
		else if (res >= 0) 
			n = &((*n)->rb_right); 
	}

	rb_link_node(&ptimer->tm_node, p, n);
	rb_insert_color(&ptimer->tm_node, &tree->root);
	tree->count++;

	if (tree->lock_enabled)
		pthread_rwlock_unlock(&tree->lock);
	return 0;
}

static inline int ptimer_is_active(ptimer_tree_t *tree, ptimer_node_t *ptimer)
{
	int ret = 1;
	if (tree->lock_enabled)
		pthread_rwlock_rdlock(&tree->lock);

	if (RB_EMPTY_NODE(&ptimer->tm_node) || ptimer->tm_node.__rb_parent_color == 0) {
		ret = 0;
	}
	if (tree->lock_enabled)
		pthread_rwlock_unlock(&tree->lock);

	return ret;
}

static inline int __ptimer_del(ptimer_tree_t *tree, ptimer_node_t *ptimer)
{
	if (RB_EMPTY_NODE(&ptimer->tm_node) || ptimer->tm_node.__rb_parent_color == 0) {
		return 0; /* return 0 currently */
	} 
		
	rb_erase(&ptimer->tm_node, &tree->root);
	RB_CLEAR_NODE(&ptimer->tm_node);
	tree->count--;
	return 0;
}

static inline int ptimer_del(ptimer_tree_t *tree, ptimer_node_t *ptimer)
{
	int ret = 0;

	if (tree->lock_enabled)
		pthread_rwlock_wrlock(&tree->lock);

	ret = __ptimer_del(tree, ptimer);

	if (tree->lock_enabled)
		pthread_rwlock_unlock(&tree->lock);

	return ret;
}

static inline void ptimer_timeout(ptimer_tree_t *tree)
{
	u_int64_t clock;
	ptimer_node_t *ptimer;
	struct rb_node *node;

	if (unlikely(!tree))
		return;

	if (tree->lock_enabled)
		pthread_rwlock_wrlock(&tree->lock);

	if (tree->count <= 0)
		goto out;

	clock = time(NULL);
	node = rb_first(&tree->root);

	while(node) {
		ptimer = rb_entry(node, ptimer_node_t, tm_node);
		if (ptimer->clock - clock > 0) {
			break;
		}

		/* delete the timeout node first before timeout_handler */ 
		__ptimer_del(tree, ptimer);
		
		if (ptimer->handler)
			ptimer->handler(ptimer->arg);

		node = rb_first(&tree->root);
	}

out:
	if (tree->lock_enabled)
		pthread_rwlock_unlock(&tree->lock);

	return;
}


#endif
