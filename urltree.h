/*
 * Date: 20171017
 *
 */



#ifndef __H_URL_TREE_H__
#define __H_URL_TREE_H__

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "urltree_hash.h"
#include "utlist.h"
#include "urltree_args.h"
#include "proxy_timer.h"


//#define UT_HASH_CACHE	1
//#define UT_HASH_CACHE_LEAF	1	/* can't open, as all intermediate node may be leaf */

#if 0
#define ut_container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif
#define ut_container_of(ptr, type, member) ({(type *)(ptr);}) 

typedef struct _ut_node {
	utlist_t sibling;	/* must be first for speed */
	utlist_head_t child;
#ifdef UT_HASH_CACHE
	unsigned int	hash_key;
	utlist_t	hash_list;
#endif
	struct _ut_node *parent;
	unsigned int 	ref;
	unsigned short level;
	unsigned short str_len;	/* less than 65535 */
	unsigned char leaf:1;
	char *str;
	
	ut_args args;
} ut_node;

typedef struct _ut_root {
	//utlist_head_t root;	/* consistent with every level search */
	ut_node *node;
	int total_node;
#ifdef UT_HASH_CACHE
	uthash_t *hash;
#endif
	ptimer_node_t	timer;
	void *msgs;
	pthread_rwlock_t lock; 
} ut_root;

extern ptimer_tree_t *dbtimer; 

/* api */
int ut_global_init();
void ut_global_free();

ut_root *ut_tree_create(char *tablename);
void ut_tree_release(ut_root *root);

/* must call ut_node_put after search */
ut_node* ut_search(ut_root *root, char *str, int len);
/* must call ut_node_put after insert */
ut_node* ut_insert(ut_root *root, char *str, int len);

int ut_delete(ut_root *root, char *str, int len);
void ut_tree_dump(ut_root *root);


void __ut_node_leaffree(ut_node *node);
static inline void ut_node_put(ut_node *node)
{
	__ut_node_leaffree(node);
}

static inline int __ut_node_get(ut_node *node)
{
	if (!node)
		return -1;
	 __sync_add_and_fetch(&node->ref, 1);
	return 0;
}


#endif
