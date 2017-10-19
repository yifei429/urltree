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
static unsigned int ut_elfhash(char *k, int len){

        unsigned int h = 0, g = 0;
        while(len > 0)
        {
                h=(h<<4)+*k++;
		if ((g = (h & 0xF0000000L)) != 0) {
                	h ^= (g>>24);
                	h &= ~g;
		}
		len--;
        }
        return (h & 0x7FFFFFFF);
}

static inline int uth_compare(utlist_t *item, utnode_info *info)
{
	ut_node *node = UTLIST_ELEM(item, typeof(node), hash_list);
	if (node->level = info->level && node->str_len == info->str_len
		&& memcpy(node->str, info->str, info->str_len)) {
		return 0;
	}
	return 1;
}
static inline unsigned int uth_keyfind(utnode_info *info, int bucket_cnt)
{
	unsigned int key = ut_elfhash(info->str, info->str_len); 
	return key % bucket_cnt;
}
static inline unsigned int uth_keyadd(utlist_t *item, int bucket_cnt)
{
	ut_node *node = UTLIST_ELEM(item, typeof(node), hash_list);
	unsigned int key = ut_elfhash(node->str, node->str_len); 
	return key % bucket_cnt;
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
	root->node = ut_node_create("/", 1, 1);
	if (!root->node)
		goto failed;

#ifdef UT_HASH_CACHE
#define ID_HASH_MAX_BUCKETS 9767
	root->hash = uthash_init(ID_HASH_MAX_BUCKETS, 1, 
		uth_keyfind, uth_keyadd, uth_compare);
	if (!root->hash)
		goto failed;
#endif

	return root;
failed:
	ut_tree_release(root);
	return NULL;
}



