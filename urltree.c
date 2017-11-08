/*
 *	Date: 2017-10-19
 *
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "urltree.h"

#ifdef UT_HASH_CACHE
static unsigned int ut_elfhash(int seed, char *k, int len)
{
	/* hash is used interation*/
    	register unsigned int hash = seed;  
    	while (len > 0){         
        	hash = hash * 131 + *k++; 
		//hash = (hash << 7) + (hash << 1) + hash + *k++;
		len--;
	}
	return hash;
}

#if 0
static inline int uth_compare(utlist_t *item, utnode_info *info)
{
	ut_node *node = UTLIST_ELEM(item, typeof(node), hash_list);
	if ((!info->level || node->level == info->level) 
		&& node->str_len == info->str_len
		&& memcpy(node->str, info->str, info->str_len)) {
		return 0;
	}
	return 1;
}
#endif
static inline void uth_dbg(utlist_t *item)
{
	ut_node *node = UTLIST_ELEM(item, typeof(node), hash_list);
	printf("(level:%d, len:%d,str:%s, parent:%s)", 
		node->level, node->str_len, node->str,
		node->parent?node->parent->str:"NULL");
	return;
}
static inline unsigned int uth_keyfind(utnode_info *info, int bucket_cnt)
{
	unsigned int key = ut_elfhash(info->parent_hash_key, info->str, info->str_len); 
	//unsigned int key = 1023456;
	//printf("find hash key:%u,len:%d,str:%s\n", key, info->str_len, info->str);
	return key % bucket_cnt;
}
static inline unsigned int uth_keyadd(utlist_t *item, int bucket_cnt)
{
	ut_node *node = UTLIST_ELEM(item, typeof(node), hash_list);
	node->hash_key = ut_elfhash(node->parent? node->parent->hash_key : 0, 
		node->str, node->str_len); 
	//printf("Add, add node, parent  key:%u, node key:%u, len:%d, str:%s\n", 
	//	node->parent? node->parent->hash_key: 0, node->hash_key, node->str_len, node->str);
	return node->hash_key % bucket_cnt;
}
#endif


void __ut_node_leaffree(ut_node *node)
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

static inline ut_node *ut_node_create(char *str, unsigned short level, 
	unsigned short size, char *str_head, int leaf)
{
	ut_node *node = NULL;
	
	if (!str || size <= 0)
		return NULL;

	node = UT_MALLOC(sizeof(ut_node));
	if (!node) {
		ut_dbg(UT_DEBUG_ERR, "urltree create node failed\n");	
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
		ut_dbg(UT_DEBUG_ERR, "del hash cache failed\n");
	}
#endif
	ut_dbg(UT_DEBUG_TRACE, "delete %d(ref:%d):%s from parent(child:%d):%d:%s\n", 
		node->str_len, node->ref, node->str, node->parent? UTLIST_HLEN(&node->parent->child):0,
		node->parent? node->parent->str_len: 0, node->parent? node->parent->str:NULL);
	if (node->parent) {
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

void ut_tree_release(ut_root *root)
{
	if (!root)
		return;

	pthread_rwlock_wrlock(&root->lock);
	ut_dbg(UT_DEBUG_INFO, "node tree release, node count:%d\n", root->total_node);
	if (root->node) {
		ut_node_free(root, root->node);
		root->node = NULL;
	}
#ifdef UT_HASH_CACHE
	if (root->hash) {
		ut_dbg(UT_DEBUG_INFO, "uthash node count:%d\n", root->hash->node_cnt);
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
		ut_dbg(UT_DEBUG_TRACE, "search node: level %d, str(len:%d):%s\n", 
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
		ut_dbg(UT_DEBUG_TRACE, "search level node->level %d, for str(len:%d):%s\n",
			node->level, end -ptr, ptr);
		node = __ut_level_search(node, ptr, end - ptr);
		if (!node || len - (end -ptr) <=  0) {
			break;
		}
		*parent = node;
		ut_dbg(UT_DEBUG_TRACE, "search level reuslt,parent(cnt:%d,level:%d:%s), for str(len:%d):%s\n",
			UTLIST_HLEN(&(*parent)->child),(*parent)->level, (*parent)->str, end -ptr, ptr);
		str = end;
		len = len - (end-ptr);
		*left = len;
		if (UTLIST_IS_HEMPTY(&node->child)) {
			ut_dbg(UT_DEBUG_TRACE, "search over, no child for node:(level:%d,str:%s)\n",
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

	ut_dbg(UT_DEBUG_TRACE, "hash confilct: len:%d, node len:%d, str:%s, nodestr:%s,level:%d,nodelevel:%d)\n",
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
		ut_dbg(UT_DEBUG_INFO, "find cache node(len:%d), str:%s\n",
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
		ut_dbg(UT_DEBUG_ERR, "wrong args for uthash search\n");
		return NULL;
	}
	//memset(&info, 0x0, sizeof(utnode_info));
	info.str = str;
	info.str_len = len;
	info.parent_hash_key = parent_hash_key;

	list = uthash_find(root->hash, &info);
	while(list) {
		node = UTLIST_ELEM(list, typeof(node), hash_list);
		ut_dbg(UT_DEBUG_INFO, "hash found node:(len:%d,leaf:%d,str:%s), allstr:%s\n", 
			node->str_len, node->leaf, node->str, str_head);
		if (ut_hash_confict(node, str, len, level, str_head) > 0) {
			return node;
		}
		i++;
		if (i > 3 && (i%3)== 0)
			ut_dbg(UT_DEBUG_WARNING, "confict count:%d\n",i);
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

	if (root->total_node >= MAX_NODES) {
		ut_dbg(UT_DEBUG_ALERT, "reach max node(%d) limit\n", MAX_NODES);
		ut_log("reach max node(%d) limit\n", MAX_NODES);
		return NULL;
	}
	if (parent)
		level = parent->level + 1;
	node = ut_node_create(str, level, len, str_head, leaf);
	if (!node) {
		ut_dbg(UT_DEBUG_ERR, "create node failed\n");
		return NULL;	
	}
	ut_dbg(UT_DEBUG_INFO, "[node] new node(level:%d,str:%s,parent:%s)\n", 
		node->level, node->str, parent? parent->str: NULL);
	if (parent) {
		UTLIST_ADD(&parent->child, &node->sibling);
		node->parent = parent;
	}
#ifdef UT_HASH_CACHE
	if (uthash_add(root->hash, &node->hash_list)) {
		ut_dbg(UT_DEBUG_ERR, "insert to hash failed\n");
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
		ut_dbg(UT_DEBUG_TRACE, "insert to parent(child:%d,level:%d:%s), %s\n", 
			parent? UTLIST_HLEN(&parent->child):0, parent?parent->level:0, 
			parent?parent->str:NULL, ptr);

		node = __ut_node_insert(root, parent, ptr, 
			end - ptr, str_head, len == (end-ptr));
		if (!node) {
			ut_dbg(UT_DEBUG_ERR, "create node failed\n");
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
ut_node* ut_insert(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	int ret = 0, left = len;
	char *str_head = str;

	if (!root)
		return NULL;

	if (!str || len <= 0 || str[0] != 47) {
		ut_dbg(UT_DEBUG_ERR, "url must start with /\n");
		return NULL;
	}
	pthread_rwlock_wrlock(&root->lock);
	ut_dbg(UT_DEBUG_TRACE, "[start] insert,search first,(len:%d)%s\n", len, str);
	node = ut_level_search(root->node, str, len, str_head, &parent, &left);
	if (node) {
		ut_dbg(UT_DEBUG_ERR, "str(%s) already exist\n", str);
		goto out;
	}
	ut_dbg(UT_DEBUG_TRACE, "start insert, not found, insert now, parent:(level:%d:%s) for (len:%d)%s\n", 
		parent?parent->level:0, parent?parent->str:NULL, len, str);

	node = ut_node_insert(root, str_head, parent, str + len - left, left);
	if (!node) {
		ut_dbg(UT_DEBUG_ERR, "insert node(str:%s) failed\n", str);
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
		ut_dbg(UT_DEBUG_TRACE, "\t");
	}
#ifdef UT_HASH_CACHE
	ut_dbg(UT_DEBUG_TRACE, "%d(child:%d, sibling:%p, hash:%u, leaf:%d):%s\n", 
		node->level, UTLIST_HLEN(&node->child), 
		sibling, node->hash_key, node->leaf, node->str);
#else 
	ut_dbg(UT_DEBUG_TRACE, "%d(child:%d, sibling:%p, leaf:%d):%s\n", 
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

void ut_tree_dump(ut_root *root)
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
	ut_dbg(UT_DEBUG_INFO, "total node:%d, max_child:%d\n", total, max_child);
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

int ut_delete(ut_root *root, char *str, int len)
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
		ut_dbg(UT_DEBUG_INFO, "delete node not found for %s\n", str);
		ret = -1;
		goto out;;
	}
	if (__ut_delete(root, node)) {
		ut_dbg(UT_DEBUG_ERR, "node delete failed(len:%d): %s\n", len, str);
		ret = -1;
		goto out;
	}

out:
	pthread_rwlock_unlock(&root->lock);
	return ret;
}


ut_root *ut_tree_create()
{
	ut_root *root;
	root = UT_MALLOC(sizeof(ut_root));
	if(!root) {
		ut_dbg(UT_DEBUG_ERR,"urltree create failed\n");	
		return NULL;
	}
	memset(root, 0x0, sizeof(ut_root));
#if 0
	root->node = ut_node_create(root_node, 1, 1, "/", 1);
	if (!root->node)
		goto failed;
	root->total_node++;
#endif
#ifdef UT_HASH_CACHE
	root->hash = uthash_init(UTHASH_MAX_BUCKETS, 0, 
		uth_keyfind, uth_keyadd, NULL, uth_dbg);
	if (!root->hash)
		goto failed;

	if (root->node && uthash_add(root->hash, &root->node->hash_list)) {
		ut_dbg(UT_DEBUG_ERR,"insert root / dir to hash failed\n");
		goto failed;
	}
#endif
	if(pthread_rwlock_init(&root->lock, NULL)) {
		ut_dbg(UT_DEBUG_ERR,"pthread init failed for tree\n");
		goto failed;
	}

	return root;
failed:
	ut_tree_release(root);
	return NULL;
}

/*
 * Return: return the tree level of the str; 0 for not found	
 */
ut_node* ut_search(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	char *ptr, *end;
	int left = 0;
	if (!root || !str || len <= 0)
		return NULL;

	pthread_rwlock_rdlock(&root->lock);
#ifdef UT_HASH_CACHE
	node = ut_hash_search(root, str, len, 0, str, 0);
	if (node) {
		//printf("find node in hash\n");
		goto out;
	} else {
		//printf("=========no node in hash\n");
	}

#endif
	node = root->node;
	node = ut_level_search(node, str, len, str, &parent, &left);
	if (node) {
		/* in rwlock, and don't add to leaf hash */
		node->leaf = 1;
		goto out;
	}
	node = NULL;
out:
	if (node) {
		__ut_node_get(node);
	}
	pthread_rwlock_unlock(&root->lock);
	return node;
}


int ut_flush2db(ut_root *root)
{
	return 0;
}



