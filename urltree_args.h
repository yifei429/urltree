/*
 *
 *
 *
 *
 */


#ifndef __H_URLTREE_ARGS_H__
#define __H_URLTREE_ARGS_H__



typedef struct _urltree_argitem {
	utlist_t list;
	unsigned int	ref;
	unsigned short name_len;
	char *name;
	void *m;
} ut_argitem;


typedef struct _urltree_args {
	utlist_head_t head;
	pthread_rwlock_t lock; 
} ut_args;

/* ======================= api =================== */
static inline int ut_init_args(ut_args *args);
static inline void ut_release_args(ut_args *args); 

static inline ut_argitem* ut_get_arg(ut_args *args, char *name, int len);
static inline void ut_put_arg(ut_argitem *item);

static inline ut_argitem* ut_insert_arg(ut_args *args, char *name, int len);

static inline int ut_arg_add_m(ut_args *args, char *name, int len, void *m);
static inline int ut_arg_del_m(ut_args *args, char *name, int len, void *m);




static inline void __ut_release_argitem(ut_argitem *item)
{
	if (!item)
		return;
	if (item->name)
		UT_FREE(item->name);

	UT_FREE(item);
	return;
}

static inline ut_argitem* __ut_create_argitem(ut_args *args, char *name, int len)
{
	ut_argitem *item;
	if (unlikely(!args || !name || len <= 0)) {
		return NULL;
	}

	item = UT_MALLOC(sizeof(ut_argitem));
	if (!item) {
		ut_err("create urltree arg failed\n");
		return NULL;
	}
	memset(item, 0x0, sizeof(ut_argitem));

	item->name = UT_MALLOC(len + 1);
	if (!item->name) {
		ut_err("create arg name failed\n");
		goto failed;
	}
	memcpy(item->name, name, len);

	__sync_fetch_and_add(&item->ref, 1);
	UTLIST_ADD(&args->head, &item->list);
	return item;

failed:
	__ut_release_argitem(item);
	return NULL;
}


static inline ut_argitem* __ut_find_arg(ut_args *args, char *name, int len)
{
	ut_argitem *item = NULL;
	utlist_t *list = NULL;
	list = args->head.n;
	while(list) {
		item = UTLIST_ELEM(list, typeof(item), list);
		if (item->name_len == len && memcmp(item->name, name, len) == 0) {
			break;
		}
		list = list->n;
	}
	
	if (list)
		return item;

	return NULL;
}


static inline ut_argitem* ut_get_arg(ut_args *args, char *name, int len)
{
	if (unlikely(!args || !name || len <= 0)) {
		return NULL;
	}
	pthread_rwlock_rdlock(&args->lock);
	ut_argitem *item = __ut_find_arg(args, name, len);
	if (item) {
		__sync_fetch_and_add(&item->ref, 1);
	}
	pthread_rwlock_unlock(&args->lock);
	return item;
}


static inline ut_argitem* ut_insert_arg(ut_args *args, char *name, int len)
{
	ut_argitem *item = NULL;
	if (unlikely(!args || !name || len <= 0)) {
		return NULL;
	}
	pthread_rwlock_wrlock(&args->lock);
	item = __ut_find_arg(args, name, len);
	if (item) {
		ut_err("arg already exist\n");
		goto out;
	}

	item = __ut_create_argitem(args, name, len);
	if (!item) {
		ut_err("replace arg module failed\n");
		goto out;
	}

out:
	pthread_rwlock_unlock(&args->lock);
	return item;
}

static inline void ut_release_argitem(ut_argitem *item)
{
	int ref = 0;
	if (unlikely(!item)) {
		return;
	}
	ref = __sync_sub_and_fetch(&item->ref, 1);
	assert(ref >= 0);
	if (ref == 0) {
		__ut_release_argitem(item);
	}
	return;
}


static inline void ut_put_arg(ut_argitem *item)
{
	ut_release_argitem(item);
	return;
}

static inline int ut_init_args(ut_args *args)
{
	if (unlikely(args))
		return -1;

	UTLIST_HINIT(&args->head);
	if(pthread_rwlock_init(&args->lock, NULL)) {
		ut_err("pthread init failed for args\n");
		return -1;
	}
	return 0;
}

static inline void ut_release_args(ut_args *args) 
{
	utlist_t *list;
	ut_argitem *item;
	if (!args)
		return;

	pthread_rwlock_wrlock(&args->lock);
	list = args->head.n;
	while(list) {
		item = UTLIST_ELEM(list, typeof(item), list);
		list = list->n;
		UTLIST_DEL(&args->head, &item->list);
		ut_release_argitem(item);
	}
	pthread_rwlock_unlock(&args->lock);
		
	return;
}

static inline int ut_arg_add_m(ut_args *args, char *name, int len, void *m)
{
	ut_argitem *item;
	int ref = 0;
	int ret = 0;
	if (unlikely(!args || !name || len <= 0 || !m)) {
		return -1;
	}

	pthread_rwlock_wrlock(&args->lock);
	item = __ut_find_arg(args, name, len);
	if (!item) {
		ut_err("no args found for the module\n");
		ret = -1;
		goto out;
	}
	if (!item->m) {
		item->m = m;
		ret = 0;
		goto out;
	}
	ut_dbg("change arg's module\n");
	UTLIST_DEL(&args->head, &item->list);
	ut_release_argitem(item);

	item = __ut_create_argitem(args, name, len);
	if (!item) {
		ut_err("replace arg module failed\n");
		goto out;
	}
	item->m = m;
	ret = 0;
out:
	pthread_rwlock_unlock(&args->lock);
	return ret;
}

static inline int ut_arg_del_m(ut_args *args, char *name, int len, void *m)
{
	ut_argitem *item;
	int ref = 0;
	if (unlikely(!args || !name || len <= 0 || !m)) {
		return -1;
	}

	pthread_rwlock_wrlock(&args->lock);
	item = __ut_find_arg(args, name, len);
	if (!item) {
		//ut_err("no args found for the module\n");
		goto out;
	}
	if (!item->m) {
		goto out;
	}
	item->m = NULL;
out:
	pthread_rwlock_unlock(&args->lock);
	return 0;
}


/* 
 * Return: 1, attack found; 0, no attack; -1, err;
 */
static inline int ut_arg_check(ut_argitem *item, char *argvalue, int len)
{
	if (!item || item->m || !argvalue || len <= 0)
		return -1;

	


	return 0;
}













#endif
