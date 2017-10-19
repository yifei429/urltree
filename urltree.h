/*
 * Date: 20171017
 *
 */



#ifndef __H_URL_TREE_H__
#define __H_URL_TREE_H__

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "urltree_hash.h"
#include "utlist.h"


#define UT_HASH_CACHE	1

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
	utlist_t		hash_list;
#endif
	struct _ut_node *parent;
	unsigned short level;
	unsigned short str_len;	/* less than 65535 */
	char *str;
} ut_node;

typedef struct _ut_root {
	//utlist_head_t root;	/* consistent with every level search */
	ut_node *node;
	int total_node;
#ifdef UT_HASH_CACHE
	uthash_t *hash;
#endif
} ut_root;



static inline void __ut_node_leaffree(ut_node *node)
{
	if (!node)
		return;
	if (node->str)
		UT_FREE(node->str);
	UT_FREE(node);

	return;
}

static inline ut_node *ut_node_create(char *str, unsigned short level, unsigned short size)
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

static inline void ut_node_free(ut_root *root, ut_node *node)
{
	ut_node *sibling = NULL, *child = NULL;
	if (!node)
		return;

	if (node->sibling.n)	
		sibling = ut_container_of(node->sibling.n, ut_node, sibling);

	if (!UTLIST_IS_HEMPTY(&node->child))
		child = ut_container_of(node->child.n, ut_node, sibling);

	if (child)	
		ut_node_free(root, child);
#ifdef UT_HASH_CACHE
	if (uthash_del(root->hash, &node->hash_list)) {
		ut_err("del hash cache failed\n");
	}
#endif
	__ut_node_leaffree(node);
	root->total_node--;
	if (sibling)
		ut_node_free(root, sibling);

	return;
}

static inline void ut_tree_release(ut_root *root)
{
	if (!root)
		return;

	ut_node_free(root, root->node);
#ifdef UT_HASH_CACHE
	printf("uthash node count:%d\n", root->hash->node_cnt);
	uthash_release(root->hash);
#endif
	UT_FREE(root);
	return;
}



ut_root *ut_tree_create();

/* ==================== action ====================== */
static inline int ut_delete()
{
	return 0;
}

static inline char *ut_str_slash(char *str, int len)
{
	assert(len >=0);
	while(len > 0) {
		if (*str == 47)
			return str + 1;
		str++;
		len--;
	}
	return str + len;
}

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
	ut_node **parent, int *left)
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

/*
 * Return: return the tree level of the str; 0 for not found	
 */
static inline int ut_search(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	char *ptr, *end;
	int left = 0;
	if (!root || !str || len <= 0)
		return -1;

	node = root->node;
	node = ut_level_search(node, str, len, &parent, &left);
	if (node) {
		return node->level;
	}
	return 0;
}

static inline ut_node* __ut_node_insert(ut_root *root, ut_node *parent, char *str, int len)
{
	ut_node *node;
	if (!parent || !str || len <=0 )
		return NULL;

	node = ut_node_create(str, parent->level + 1, len);
	if (!node) {
		ut_err("create node failed\n");
		return NULL;	
	}
	ut_dbg("[node] new node(level:%d,str:%s,parent:%s)\n", 
		node->level, node->str, parent->str);
	UTLIST_ADD(&parent->child, &node->sibling);
	node->parent = parent;
#ifdef UT_HASH_CACHE
	if (uthash_add(root->hash, &node->hash_list)) {
		ut_err("insert to hash failed\n");
	}
#endif
	root->total_node++;
	return node;
}

static inline int ut_node_insert(ut_root *root, ut_node *parent, char *str, int len)
{
	ut_node *node, *p;
	char *ptr = NULL, *end = NULL;
	if (!parent || !str || len <=0 )
		return -1;

	p = parent;
	while(len > 0) {
		ptr = str;
		end = ut_str_slash(str, len);
		ut_dbg("insert to parent(child:%d,level:%d:%s), %s\n", 
			UTLIST_HLEN(&parent->child), parent->level, parent->str, ptr);
		node = __ut_node_insert(root, parent, ptr, end - ptr);
		if (!node) {
			ut_err("create node failed\n");
			return -1;	
		}
		if (len - (end - ptr) <= 0)
			break;
		len = len - (end - ptr);
		parent = node;
		str = end;

	}

	return 0;
}

static inline int ut_insert(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	int ret = 0, left;

	ut_dbg("[start] insert,search first,(len:%d)%s\n", len, str);
	node = ut_level_search(root->node, str, len, &parent, &left);
	if (node) {
		ut_dbg("str(%s) already exist\n", str);
		return 0;
	}
	if (!parent) {
		ut_err("failed find parent\n");
		return -1;
	}
	ut_dbg("start insert, not found, insert now, parent:(level:%d:%s) for (len:%d)%s\n", 
		parent->level, parent->str, len, str);

	ret = ut_node_insert(root, parent, str + len - left, left);
	if (ret) {
		ut_err("insert node(str:%s) failed\n", str);
	}

	return ret;
}

static inline void ut_node_dump(ut_node *node, int *cnt)
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
	printf("%d(child:%d, sibling:%p):%s\n", 
		node->level, UTLIST_HLEN(&node->child), sibling, node->str);
	*cnt = *cnt + 1;

	if (child) {	
		ut_node_dump(child, cnt);
	}
	if (sibling) {
		ut_node_dump(sibling, cnt);
	}

	return;
}
static inline void ut_tree_dump(ut_root *root)
{
	int total = 0;
	if (!root || !root->node)
		return;

	ut_node_dump(root->node, &total);
	printf("total node:%d\n", total);
#ifdef UT_HASH_CACHE
	uthash_dump(root->hash);
#endif
	return;
}

#endif
