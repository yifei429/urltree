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


ut_root *ut_tree_create()
{
	ut_root *root;
	root = UT_MALLOC(sizeof(ut_root));
	if(!root) {
		ut_err("urltree create failed\n");	
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
		ut_err("insert root / dir to hash failed\n");
		goto failed;
	}
#endif
	if(pthread_rwlock_init(&root->lock, NULL)) {
		ut_err("pthread init failed for tree\n");
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



