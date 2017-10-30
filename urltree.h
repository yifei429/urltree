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
	pthread_rwlock_t lock; 
} ut_root;

/* api */
ut_root *ut_tree_create();
static inline void ut_tree_release(ut_root *root);

/* must call ut_node_put after search */
ut_node* ut_search(ut_root *root, char *str, int len);
/* must call ut_node_put after insert */
static inline ut_node* ut_insert(ut_root *root, char *str, int len);

static inline int ut_delete(ut_root *root, char *str, int len);
static inline void ut_tree_dump(ut_root *root);
int ut_flush2db(ut_root *root);

static inline void ut_node_put(ut_node *node);




static inline void __ut_node_leaffree(ut_node *node)
{
	int ref = 0;
	if (!node)
		return;
	ref = __sync_sub_and_fetch(&node->ref, 1);
	if (ref > 0)
		return;
	if (node->str)
		UT_FREE(node->str);

	ut_release_args(&node->args);

	UT_FREE(node);

	return;
}

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

static inline ut_node *ut_node_create(char *str, unsigned short level, 
	unsigned short size, char *str_head, int leaf)
{
	ut_node *node = NULL;
	
	if (!str || size <= 0)
		return NULL;

	node = UT_MALLOC(sizeof(ut_node));
	if (!node) {
		ut_err("urltree create node failed\n");	
		return NULL;
	}
	memset(node, 0x0, sizeof(ut_node));
	node->level = level;
	UTLIST_EINIT(&node->sibling);
	UTLIST_HINIT(&node->child);
	node->leaf = leaf;
	ut_init_args(&node->args);
	__sync_add_and_fetch(&node->ref, 1);

	if (str) {
		node->str = UT_MALLOC(size + 1);
		if (!node->str)
			goto failed;
		memcpy(node->str, str, size);
		node->str[size] = '\0';
		node->str_len = size;
	}

	return node;
failed:
	__ut_node_leaffree(node);
	return NULL;
}

/* must be called in lock */
static inline void ut_node_free(ut_root *root, ut_node *node);
/* must be called in lock */
static inline void __ut_node_free(ut_root *root, ut_node *node) 
{
	ut_node *child = NULL;
	if (!node)
		return;

	if (!UTLIST_IS_HEMPTY(&node->child)) {
		child = ut_container_of(node->child.n, ut_node, sibling);
		ut_node_free(root, child);
	}
#ifdef UT_HASH_CACHE
	if (uthash_del(root->hash, &node->hash_list)) {
		ut_err("del hash cache failed\n");
	}
#endif
	if (node->parent) {
		ut_dbg("delete %d:%s from parent(child:%d):%d:%s\n", 
			node->str_len, node->str, UTLIST_HLEN(&node->parent->child),node->parent->str_len, node->parent->str);
		assert(!UTLIST_IS_UNLINK(&node->sibling));
		UTLIST_DEL(&node->parent->child, &node->sibling);
	}
		
	__ut_node_leaffree(node);
	root->total_node--;
	return;
}

/* must be called in lock */
static inline void ut_node_free(ut_root *root, ut_node *node)
{
	ut_node *sibling = NULL, *child = NULL;
	if (!node)
		return;

	while (node) {
		sibling = NULL;
		if (node->sibling.n)	
			sibling = ut_container_of(node->sibling.n, ut_node, sibling);
		__ut_node_free(root, node);
		node = sibling;
	}
	return;
}

static inline void ut_tree_release(ut_root *root)
{
	if (!root)
		return;

	pthread_rwlock_wrlock(&root->lock);
	if (root->node) {
		ut_node_free(root, root->node);
		root->node = NULL;
	}
#ifdef UT_HASH_CACHE
	if (root->hash) {
		//printf("uthash node count:%d\n", root->hash->node_cnt);
		uthash_release(root->hash);
		root->hash = NULL;
	}
#endif
	pthread_rwlock_unlock(&root->lock);
	UT_FREE(root);
	return;
}



ut_root *ut_tree_create();

/* ==================== action ====================== */
static inline ut_node *__ut_level_search(ut_node *node, char *str, int len)
{
	ut_node *bak = node;
	while (node) {
		ut_dbg("search node: level %d, str(len:%d):%s\n", 
		node->level, node->str_len, node->str);
		if (node->str_len == len && memcmp(node->str, str, len) == 0) {
			break;
		}
		if (node->sibling.n)
			node = ut_container_of(node->sibling.n, ut_node, sibling);
		else {
			node = NULL;
			break;
		}
	}
	return node;
}
static inline ut_node *ut_level_search(ut_node *node, char *str, int len, 
	char *str_head, ut_node **parent, int *left)
{
	char *ptr, *end;
	int level = 0;
	if (!node || !parent || !str || len <= 0)
		return NULL;

	*parent = node->parent;
	*left = len;
	while(node) {
		ptr = str;
		end = ut_str_slash(str, len);
		ut_dbg("search level node->level %d, for str(len:%d):%s\n",
			node->level, end -ptr, ptr);
		node = __ut_level_search(node, ptr, end - ptr);
		if (!node || len - (end -ptr) <=  0) {
			break;
		}
		*parent = node;
		ut_dbg("search level reuslt,parent(cnt:%d,level:%d:%s), for str(len:%d):%s\n",
			UTLIST_HLEN(&(*parent)->child),(*parent)->level, (*parent)->str, end -ptr, ptr);
		str = end;
		len = len - (end-ptr);
		*left = len;
		if (UTLIST_IS_HEMPTY(&node->child)) {
			ut_dbg("search over, no child for node:(level:%d,str:%s)\n",
				node->level, node->str);
			node = NULL;
			break;
		}
		node = ut_container_of(node->child.n, ut_node, sibling); 
	}
	
	return node;
}

#ifdef UT_HASH_CACHE
static inline int ut_hash_confict(ut_node *node, 
	char *str, int len, int level, char *str_head)
{
	if (!node)
		return 0;

	ut_dbg("hash confilct: len:%d, node len:%d, str:%s, nodestr:%s,level:%d,nodelevel:%d)\n",
		len, node->str_len, str, node->str, level, node->level);

	if (level && (node->str_len != len || node->level != level))
		return 0;
	str = str + len;
	do {
		str = str - node->str_len;
		if (str < str_head)
			break;
		if (memcmp(node->str, str, node->str_len) != 0) {
			break;
		}
		node = node->parent;
	} while (node);

	if (!node && str_head == str) {
		ut_dbg("find cache node(len:%d), str:%s\n",
			node->str_len, node->str);
		return 1;
	}
	return 0;
}

static inline ut_node* ut_hash_search(ut_root *root, 
	char *str, int len, int level, 
	char *str_head, unsigned int parent_hash_key)
{
	utnode_info info;
	utlist_t *list;
	ut_node *node;
	int i = 0;
	if (!root->hash || !str || len < 0) {
		ut_err("wrong args for uthash search\n");
		return NULL;
	}
	//memset(&info, 0x0, sizeof(utnode_info));
	info.str = str;
	info.str_len = len;
	info.parent_hash_key = parent_hash_key;

	list = uthash_find(root->hash, &info);
	while(list) {
		node = UTLIST_ELEM(list, typeof(node), hash_list);
		ut_dbg("hash found node:(len:%d,leaf:%d,str:%s), allstr:%s\n", 
			node->str_len, node->leaf, node->str, str_head);
		if (ut_hash_confict(node, str, len, level, str_head) > 0) {
			return node;
		}
		i++;
		//if (i > 5)
		//	printf("confict =====\n");
		list = list->n;
	}

	return NULL;
}
#endif

static inline char *ut_last_node(char *str, int len)
{
	if (!str || len <= 0)
		return NULL;
	if (str[len -1 ] == 47) {
		len--;
	}
	while (len > 0) {
		if (str[len - 1] == 47)
			break;
		len--;
	}
	return str + len;
}

static inline ut_node* __ut_node_insert(ut_root *root, ut_node *parent, 
	char *str, int len, char *str_head, int leaf)
{
	ut_node *node;
	int level = 1;
	if (!str || len <=0 )
		return NULL;

	if (parent)
		level = parent->level + 1;
	node = ut_node_create(str, level, len, str_head, leaf);
	if (!node) {
		ut_err("create node failed\n");
		return NULL;	
	}
	ut_dbg("[node] new node(level:%d,str:%s,parent:%s)\n", 
		node->level, node->str, parent? parent->str: NULL);
	if (parent) {
		UTLIST_ADD(&parent->child, &node->sibling);
		node->parent = parent;
	}
#ifdef UT_HASH_CACHE
	if (uthash_add(root->hash, &node->hash_list)) {
		ut_err("insert to hash failed\n");
	}
#endif
	root->total_node++;
	return node;
failed:
	if (node) {
		__ut_node_leaffree(node);
	}
	return NULL;
}

static inline ut_node* ut_node_insert(ut_root *root, 
	char *str_head, ut_node *parent, 
	char *str, int len)
{
	ut_node *node = NULL;
	char *ptr = NULL, *end = NULL;
	if (!str || len <=0 )
		return NULL;

	while(len > 0) {
		ptr = str;
		end = ut_str_slash(str, len);
		ut_dbg("insert to parent(child:%d,level:%d:%s), %s\n", 
			parent? UTLIST_HLEN(&parent->child):0, parent?parent->level:0, 
			parent?parent->str:NULL, ptr);

		node = __ut_node_insert(root, parent, ptr, 
			end - ptr, str_head, len == (end-ptr));
		if (!node) {
			ut_err("create node failed\n");
			return NULL;	
		}
		if (unlikely(!root->node))
			root->node = node;

		if (len - (end - ptr) <= 0)
			break;
		len = len - (end - ptr);
		parent = node;
		str = end;

	}

	return node;
}

/* return: 1, if exist; 0, OK; -1, error */
static inline ut_node* ut_insert(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	int ret = 0, left = len;
	char *str_head = str;

	if (!root)
		return NULL;

	if (!str || len <= 0 || str[0] != 47) {
		ut_err("url must start with /\n");
		return NULL;
	}
	pthread_rwlock_wrlock(&root->lock);
	ut_dbg("[start] insert,search first,(len:%d)%s\n", len, str);
	node = ut_level_search(root->node, str, len, str_head, &parent, &left);
	if (node) {
		ut_dbg("str(%s) already exist\n", str);
		goto out;
	}
	ut_dbg("start insert, not found, insert now, parent:(level:%d:%s) for (len:%d)%s\n", 
		parent?parent->level:0, parent?parent->str:NULL, len, str);

	node = ut_node_insert(root, str_head, parent, str + len - left, left);
	if (!node) {
		ut_err("insert node(str:%s) failed\n", str);
		ret = -1;
		goto out;
	}
out:
	if (node) {
		__ut_node_get(node);
	}
	pthread_rwlock_unlock(&root->lock);
	return node;
}

static inline void ut_node_dump(ut_node *node, int *cnt, int *max_child)
{
	ut_node *sibling = NULL, *child = NULL;
	int i = 0;
	if (!node)
		return;
	if (node->sibling.n)	
		sibling = ut_container_of(node->sibling.n, ut_node, sibling);

	if (!UTLIST_IS_HEMPTY(&node->child))
		child = ut_container_of(node->child.n, ut_node, sibling);

	for (i = 0; i < node->level - 1; i++) {
		printf("\t");
	}
#ifdef UT_HASH_CACHE
	printf("%d(child:%d, sibling:%p, hash:%u, leaf:%d):%s\n", 
		node->level, UTLIST_HLEN(&node->child), 
		sibling, node->hash_key, node->leaf, node->str);
#else 
	printf("%d(child:%d, sibling:%p, leaf:%d):%s\n", 
		node->level, UTLIST_HLEN(&node->child), 
		sibling, node->leaf, node->str);
#endif
		
	*cnt = *cnt + 1;
	*max_child = (*max_child > UTLIST_HLEN(&node->child))? (*max_child) :UTLIST_HLEN(&node->child);

	if (child) {	
		ut_node_dump(child, cnt, max_child);
	}
	if (sibling) {
		ut_node_dump(sibling, cnt, max_child);
	}

	return;
}
static inline void ut_tree_dump(ut_root *root)
{
	int total = 0, max_child = 0;
	if (!root)
		return;

	pthread_rwlock_wrlock(&root->lock);
	if (root->node) 
		ut_node_dump(root->node, &total, &max_child);
#ifdef UT_HASH_CACHE
	if (root->hash)
		uthash_dump(root->hash);
#endif
	printf("total node:%d, max_child:%d\n", total, max_child);
	pthread_rwlock_unlock(&root->lock);
	return;
}

static inline int __ut_delete(ut_root *root, ut_node *node)
{
	ut_node *parent;
	if (!root || !node)
		return -1;

	__ut_node_free(root, node);
	
	if (root->node == node) {
		root->node = NULL;
	}

	return 0;
}

static inline int ut_delete(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	int left;
	char *str_head = str;
	int ret = 0;

	if (!root || !str || len <= 0)
		return -1;

	pthread_rwlock_wrlock(&root->lock);
	node = ut_level_search(root->node, str, len, str_head, &parent, &left);
	if (!node) {
		ut_dbg("delete node not found for %s\n", str);
		ret = -1;
		goto out;;
	}
	if (__ut_delete(root, node)) {
		ut_err("node delete failed(len:%d): %s\n", len, str);
		ret = -1;
		goto out;
	}

out:
	pthread_rwlock_unlock(&root->lock);
	return ret;
}



#endif
