/*
 * Date: 20171017
 *
 *
 */


#ifndef __H_URL_TREE_HASH_H__
#define __H_URL_TREE_HASH_H__

#include <assert.h>
#include <pthread.h>

#include "utlist.h"

/* for search */
typedef struct _utnode_info {
	char *str;
	unsigned short str_len;
	unsigned short level;
} utnode_info;

typedef struct _utnode_head_t {
	utlist_head_t head;
	pthread_rwlock_t lock;
} utnode_head_t;

typedef int (*uthash_compare) (utlist_t *item1, utnode_info *info);
typedef unsigned int (*uthash_keyfind) (utnode_info *info, int bucket_cnt);
typedef unsigned int (*uthash_keyadd) (utlist_t *item, int bucket_cnt);
typedef void (*uthash_dbg) (utlist_t *item);

#define ID_HASH_MAX_BUCKETS 99767
typedef struct _uthash_t {
        int max_size;
        int node_cnt;
        int thread_safe;

        uthash_compare compare;
	uthash_keyfind keyfind;
	uthash_keyadd keyadd;
	uthash_dbg dbg;

        //ut_head_t list;
        //pthread_rwlock_t list_lock;

        utnode_head_t table[0];
} uthash_t;

static inline uthash_t* uthash_init(int size, int thread_safe,
	uthash_keyfind keyfind, uthash_keyadd keyadd,
	uthash_compare compare, uthash_dbg dbg)
{
        uthash_t *hash = NULL;
        int len = 0;
        int i = 0;

        if (size <=0 || !keyfind || !keyadd || !compare)
                return NULL;

        len = sizeof(uthash_t) + sizeof(utnode_head_t) * size;
        hash = UT_MALLOC(len);
        if (!hash)
                return NULL;

        memset(hash, 0x0, len);

        hash->thread_safe = thread_safe;
        if (hash->thread_safe) {
                for (i = 0; i < size; i++) {
                        if (pthread_rwlock_init(&hash->table[i].lock, NULL) != 0) {
                                printf("init id hash lock failed.\n");
                                goto failed;
                        }
                }
#if 0
                if (pthread_rwlock_init(&hash->list_lock, NULL) != 0) {
                        printf("init id hash list lock failed.\n");
                        goto failed;
                }
#endif
        }

        hash->max_size = size;
        hash->node_cnt = 0;
        hash->compare = compare;
        hash->keyfind = keyfind;
        hash->keyadd = keyadd;
        hash->dbg = dbg;

        return hash;
failed:
	if (hash)
		UT_FREE(hash);
	return NULL;
}

static inline int uthash_add(uthash_t *hash, utlist_t *item)
{
        unsigned int key = 0;
        utnode_head_t *head = NULL;

        if (unlikely(!hash || !item))
                return -1;

        key = hash->keyadd(item, hash->max_size);
        head = &hash->table[key];

	if (hash->thread_safe)
		pthread_rwlock_wrlock(&head->lock);
	UTLIST_ADD(&head->head, item);
	if (UTLIST_HLEN(&head->head) > 20) {
		printf("======> hash confict(%d) more than 3.\n",
			UTLIST_HLEN(&head->head));
		utlist_t *item1 = NULL;
        	item1 = head->head.n;
        	while(item1) {
			printf("\t");
			hash->dbg(item1);
			printf("\n");
                	item1 = item1->n;
        	}
	}
	hash->node_cnt++;
	if (hash->thread_safe)
		pthread_rwlock_unlock(&head->lock);
out:
	return 0;
}

static inline int uthash_del(uthash_t *hash, utlist_t *item)
{
        unsigned int key = 0;
        utnode_head_t *head = NULL;

        if (unlikely(!hash || !item))
                return -1;

	if (UTLIST_IS_UNLINK(item)) {
		ut_err("item is not linked in hash\n");
		return -1;
	}

        key = hash->keyadd(item, hash->max_size);
        head = &hash->table[key];

	if (hash->thread_safe)
		pthread_rwlock_wrlock(&head->lock);
	UTLIST_DEL(&head->head, item);
	hash->node_cnt--;
	if (hash->thread_safe)
		pthread_rwlock_unlock(&head->lock);
out:
	return 0;
}

static inline utlist_t *__uthash_find(uthash_t *hash, utnode_info *info, unsigned int key)
{
        utnode_head_t *head = &hash->table[key];
        utlist_t *item = NULL;

	if (hash->thread_safe)
		pthread_rwlock_rdlock(&head->lock);
        item = head->head.n;
        while(item) {
                if (!hash->compare(item, info))
                        break;

                item = item->n;
        }

	if (hash->thread_safe)
		pthread_rwlock_unlock(&head->lock);
        return item;
}

static inline utlist_t* uthash_find(uthash_t *hash, utnode_info *info)
{
        unsigned int key = 0;
        utlist_t *item;

        if (unlikely(!hash || !info))
                return NULL;

        key = hash->keyfind(info, hash->max_size);
        if (!(item = __uthash_find(hash, info, key))) {
                goto out;
        }

        return item;
out:
        return NULL;

}

static inline void uthash_release(uthash_t *hash)
{
	int i = 0;

	if (!hash)
		return;

	if (hash->node_cnt > 0)
		ut_err("release hash with nodes\n");	
	assert(hash->node_cnt == 0);

	UT_FREE(hash);

	return;
}

static inline void uthash_dump(uthash_t *hash)
{
	int i = 0;
	utnode_head_t *head;
        utlist_t *item = NULL;

	if (!hash)
		return;
	
	for (i = 0; i < hash->max_size; i++) {
		head = &hash->table[i];	

		if (hash->thread_safe)
			pthread_rwlock_rdlock(&head->lock);
        	item = head->head.n;
        	while(item) {
        	        item = item->n;
			/*debug item info */
        	}

		if (hash->thread_safe)
			pthread_rwlock_unlock(&head->lock);
	}

	printf("hash node count:%d\n", hash->node_cnt);
	return;
}

#endif
