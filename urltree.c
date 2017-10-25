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
#if 0
        unsigned int h = seed, g = 0;
        while(len > 0)
        {
                h=(h<<4)+*k++;
		if ((g = (h & 0xF0000000L)) != 0) {
                	h ^= (g>>24);
                	h &= ~g;
		}
		len--;
        }
        //return (h & 0x7FFFFFFF);
	return h;
#endif
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
#ifdef UT_HASH_CACHE_LEAF
	node->hash_key = ut_elfhash(0, node->allstr, node->allstr_len); 
#else
	node->hash_key = ut_elfhash(node->parent? node->parent->hash_key : 0, 
		node->str, node->str_len); 
#endif
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
int ut_search(ut_root *root, char *str, int len)
{
	ut_node *node, *parent = NULL;
	char *ptr, *end;
	char *str_node = str;
	int left = 0, ret = 0;
	if (!root || !str || len <= 0)
		return -1;

	pthread_rwlock_rdlock(&root->lock);
#ifdef UT_HASH_CACHE
#ifdef UT_HASH_CACHE_LEAF 
#else
	str_node = ut_last_node(str, len);
	if (!str_node) {
		ut_err("wrong url str(len:%d):%s, must start with slash \n", len, str);
		ret = -1;
		goto out;
	}
#endif
	node = ut_hash_search(root, str_node, len - (str_node -str), 0, str, 
		ut_elfhash(0, str, str_node - str));
	if (node) {
		ret = node->level;
		goto out;
	}

#endif
	node = root->node;
	node = ut_level_search(node, str, len, str, &parent, &left);
	if (node) {
		ret = node->level;
		goto out;
	}

out:
	pthread_rwlock_unlock(&root->lock);
	return ret;
}



