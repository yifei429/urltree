/*
 *
 *
 *
 */

#ifndef __H_URL_CBTREE_H__
#define __H_URL_CBTREE_H__

#include <stdint.h> 
#include <string.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <errno.h> 
#include <unistd.h>

#include "utlist.h"
#define uint32 unsigned int

typedef struct _ucb_subtree {
	void *root;
 	struct _ucb_subtree *parent;
	utlist_t      list;
	utlist_head_t leaf_head;
	unsigned char *str;
	unsigned short str_len;
	unsigned short level;
} ucb_subtree;

typedef struct {
	void *root;
	//int leaf_cnt;
} ucb_tree;

typedef struct {
	void *child[2];
	uint32 byte;
	unsigned char otherbits;
} ucb_subnode;

int max_depth = 0;
static inline ucb_subtree *
__ucb_subtree_search(void *t, const char *u, int u_len)
{
	unsigned char *p= t;
	ucb_subtree *leaf = NULL;
	unsigned char c= 0;
	int direction;
	ucb_subnode *q;
	int dep = 0;

	if(!p)
		return 0;

	while(1&(intptr_t)p){
		dep++;
		q = (void*)(p-1);
		c = 0;
		if (q->byte < u_len) { 
			c = u[q->byte];
			direction= (1+(q->otherbits|c))>>8;
		} else {
			direction= (1+q->otherbits)>>8;
		}

		p= q->child[direction];
	}

	if (dep > max_depth)
		max_depth = dep;

	leaf = (ucb_subtree *)p;
	ut_dbg(UT_DEBUG_INFO, "get leaf(level:%d, depth:%d):%d:%s, for %d:%s\n", 
		leaf->level, dep, leaf->str_len, leaf->str, u_len, u);
	if (leaf->str_len == u_len && 0 == memcmp(u,leaf->str, u_len)) {
		return leaf; 
	}
	return NULL;
}

static inline void ucb_subtree_release_leaf(ucb_subtree* leaf)
{
	if (!leaf)
		return;
	if (leaf->str)
		UT_FREE(leaf->str);
	if (leaf->parent)
		UTLIST_DEL(&leaf->parent->leaf_head, &leaf->list);

	UT_FREE(leaf);
	return;
}

static inline ucb_subtree* ucb_subtree_create_leaf(ucb_subtree *parent, const char *u, int u_len, int level)
{
	ucb_subtree *t = NULL;
	int ret = 0;
	ret = UT_MALLOC_ALIGN(((void **)(&t)), sizeof(void *), sizeof(ucb_subtree));	
	if (ret)
		return NULL;

	memset(t, 0x0, sizeof(ucb_subtree));
	if (u_len > 0 && u) {
		t->str_len = u_len;
		t->str = UT_MALLOC(u_len + 1);
		if (!t->str) {
			UT_FREE(t);
			return NULL;
		}
		memcpy(t->str, u, u_len);
	}
	UTLIST_HINIT(&t->leaf_head);
	UTLIST_EINIT(&t->list);
	t->level = level;
	if (parent) {
		UTLIST_ADD(&parent->leaf_head, &t->list);
		t->parent = parent;
	}
	
	return t;
}	


/*
 */
static inline ucb_subtree* __ucb_subtree_insert(ucb_subtree *t, const char *u, int u_len)
{
	const unsigned char*const ubytes= (void*)u;
	const size_t ulen= u_len;
	unsigned char* p= t->root;
	ucb_subtree *leaf = NULL;
	ucb_subtree *sf = NULL;

	if(!p){
		leaf = ucb_subtree_create_leaf(t, u , ulen, t->level + 1);
		if(!leaf) {
			ut_dbg(UT_DEBUG_ERR,"url subtree create failed\n");
			return NULL;
		}
		t->root = leaf;
		ut_dbg(UT_DEBUG_INFO, "insert new leaf to tree(child:%d, str:%s),leaf:%p, for %d:%s\n", 
			t->leaf_head.cnt, t->str, leaf, u_len, u);
		return leaf;
	}

	while(1&(intptr_t)p){
		ucb_subnode*q= (void*)(p-1);

		unsigned char c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		const int direction= (1+(q->otherbits|c))>>8;

		p= q->child[direction];
	}

	uint32 newbyte;
	uint32 newotherbits;

	sf = (ucb_subtree *)p;
	for(newbyte= 0;newbyte<ulen;++newbyte){
		if(sf->str[newbyte]!=ubytes[newbyte]){
			newotherbits= sf->str[newbyte]^ubytes[newbyte];
			goto different_byte_found;
		}
	}

	if(sf->str[newbyte]!=0){
		newotherbits= sf->str[newbyte];
		goto different_byte_found;
	}
	ut_dbg(UT_DEBUG_ERR,"node already exist\n");
	return NULL;

different_byte_found:
	/* get the highest different bit*/
	newotherbits|= newotherbits>>1;
	newotherbits|= newotherbits>>2;
	newotherbits|= newotherbits>>4;
	newotherbits= (newotherbits&~(newotherbits>>1))^255;
	/* compute direction*/
	unsigned char c= sf->str[newbyte];
	int newdirection= (1+(newotherbits|c))>>8;

	ucb_subnode*newnode;
	if(UT_MALLOC_ALIGN((void**)&newnode,sizeof(void*),sizeof(ucb_subnode))) {
		ut_dbg(UT_DEBUG_ERR,"url subtree create failed\n");
		return NULL;
	}

	leaf = ucb_subtree_create_leaf(t, u , ulen, t->level + 1);
	if(!leaf) {
		ut_dbg(UT_DEBUG_ERR,"url subtree create failed\n");
		UT_FREE(newnode);
		return NULL;
	}

	ut_dbg(UT_DEBUG_INFO, "insert new leaf to tree(child:%d,str:%s):leaf:%p(newnode:%p),%d:%s\n", 
		t->leaf_head.cnt,t->str, sf, newnode, u_len, u);
	newnode->byte= newbyte;
	newnode->otherbits= newotherbits;
	newnode->child[1-newdirection]= leaf;

	void**wherep= &t->root;
	for(;;){
		unsigned char*p= *wherep;
		if(!(1&(intptr_t)p))break;
		ucb_subnode*q= (void*)(p-1);
		if(q->byte> newbyte)break;
		if(q->byte==newbyte&&q->otherbits> newotherbits)break;
		unsigned char c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		const int direction= (1+(q->otherbits|c))>>8;
		wherep= q->child+direction;
	}

	newnode->child[newdirection]= *wherep;
	*wherep= (void*)(1+(char*)newnode);

	return leaf;
}



static inline ucb_subtree *
ucb_subtree_search(ucb_subtree *t, const char *u, int len, ucb_subtree **parent, int *left)
{
	ucb_subtree *leaf = NULL;
	const char *ptr, *end;
	if (!t || !u || len <= 0 || !parent || !left) {
		ut_dbg(UT_DEBUG_ERR,"wrong args:t:%p,u:%p,len:%d,parent:%p,left:%p\n",
			t, u, len, parent, left);
		return NULL;
	}
	*parent = t;
	*left = len;
	while(t->root) {
		ptr = u;
		end = ut_str_slash((char *)ptr, len);
		ut_dbg(UT_DEBUG_TRACE, "search tree:(len:%d)%s, root:%p, for len:%d,%s\n",
			t->str_len, t->str, t->root, end-ptr, ptr);
		leaf = __ucb_subtree_search(t->root, ptr, end - ptr);
		if (!leaf) {
			ut_dbg(UT_DEBUG_TRACE, "not find in parent(level:%d)%d:%s. for %d:%s\n", 
				t->level, t->str_len, t->str, end-ptr, ptr);
			return NULL;
		}
	
		if (len - (end-ptr) <= 0) {
			ut_dbg(UT_DEBUG_TRACE, "[find leaf] (len:%d, root:%p):%s, for (len:%d) %s\n", 
				leaf->str_len, leaf->root, leaf->str, end-ptr, ptr);
			break;
		}
		ut_dbg(UT_DEBUG_TRACE, "find next tree(len:%d, root:%p):%s, for (len:%d) %s\n", 
			leaf->str_len, leaf->root, leaf->str, end-ptr, ptr);
		t = leaf;
		*parent = t;
		u = end;
		len = len - (end-ptr);
		*left = len;
	}
	return leaf;
}

/*
 * Return: 1, if found; err, others;
 */
static inline int ucbtree_search(ucb_tree *t, char *u, int len)
{
	ucb_subtree *st, *parent;
	int left = 0;
	if (unlikely(!t || !t->root || !u || len <= 0))
		return -1;

	st = t->root;
	if (memcmp(u, st->str, 1) != 0)
		return -1;
	if (len <= 1) {
		return 1;
	}
	st = ucb_subtree_search(st, u+1, len-1, &parent, &left); 
	if (!st) {
		ut_dbg(UT_DEBUG_TRACE, "search for(len:%d) %s failed\n", len, u);
		return -1;
	}
	ut_dbg(UT_DEBUG_TRACE, "search for(len:%d) %s success\n", len, u);

	return 1;
}

static inline void ucb_subtree_release(ucb_subtree *st);
static inline void
__ucb_subtree_release(void*top)
{
	unsigned char *p= top;

	if(1&(intptr_t)p){
		ucb_subnode*q= (void*)(p-1);
		__ucb_subtree_release(q->child[0]);
		__ucb_subtree_release(q->child[1]);
		UT_FREE(q);
	}else{
		ucb_subtree_release((ucb_subtree *)p);
	}
}

static inline void ucb_subtree_release(ucb_subtree *st)
{
	unsigned char *p;
	if (!st)
		return;

	if (st->root) {
		__ucb_subtree_release(st->root);
	}
	ucb_subtree_release_leaf(st);
	return;
}

static inline void ucb_tree_release(ucb_tree *t)
{
	if (!t)
		return;
	if (t->root) {
		ucb_subtree_release(t->root);
	}
	UT_FREE(t);
}


static inline void ucb_subtree_dump(ucb_subtree *t, int *cnt);
static inline void
__ucb_subtree_dump(void*top, int *cnt)
{
	unsigned char *p= top;

	if(1&(intptr_t)p){
		ucb_subnode*q= (void*)(p-1);
		__ucb_subtree_dump(q->child[0], cnt);
		__ucb_subtree_dump(q->child[1], cnt);
	}else{
		ucb_subtree_dump((ucb_subtree *)p, cnt);
	}
}

static inline void ucb_subtree_dump(ucb_subtree *t, int *cnt)
{
	utlist_t *list;
	ucb_subtree *t1;
	int i;
	if (!t)
		return;

	*cnt = *cnt + 1;
	for (i = 0; i < t->level - 1; i++) {
		printf("\t");
	}
	printf("%d(child:%d), url(len:%d):%s\n",
		t->level, UTLIST_HLEN(&t->leaf_head),
		t->str_len, t->str);

	if (!t->root) {
		return;
	}
	__ucb_subtree_dump(t->root, cnt);
#if 0
	if (UTLIST_IS_HEMPTY(&t->leaf_head))
		return;
	list = t->leaf_head.n;
	while (list) {
		t1 = UTLIST_ELEM(list, typeof(t1), list);
		ucb_subtree_dump(t1);
		list = list->n;
	}
#endif
}

static inline void ucb_tree_dump(ucb_tree *t)
{
	int total_cnt = 0;
	if (!t || !t->root)
		return;
	
	if (t->root) {
		ucb_subtree_dump(t->root, &total_cnt);
	}
	printf("=========== total tree leaf:%d===========\n", total_cnt);

	return;
}

static inline ucb_tree *ucb_tree_create()
{
	ucb_tree *t;
	char *rooturl = "/";
	t = UT_MALLOC(sizeof(ucb_tree));
	if (!t) {
		ut_dbg(UT_DEBUG_ERR,"ucb tree create failed\n");
		return NULL;
	}
	t->root = ucb_subtree_create_leaf(NULL, rooturl, strlen(rooturl), 1); 
	if (!t->root) {
		UT_FREE(t);
		return NULL;
	}
	//t->leaf_cnt++;
	return t;
}


static inline int ucb_subtree_insert(ucb_subtree *st, const char *u, int len)
{
	const char *ptr, *end;
	ucb_subtree *leaf;
	if (unlikely(!st || !u || len <= 0))
		return -1;

	while (len > 0) {
		ptr = u;
		end = ut_str_slash((char *)u, len); 

		leaf = __ucb_subtree_insert(st, ptr, end - ptr);
		if (!leaf)
			break;
		leaf->parent = st;
		len = len - (end-ptr);
		u = end;
		st = leaf;
	}
	if (!leaf)
		return -1;

	return 0;
}

/*
 * Return:1,exist; 0, success; -1, err;
 */
static inline int ucbtree_insert(ucb_tree *t, const char *u, int len)
{
	ucb_subtree *st, *parent;
	int left = 0;
	int ret = 0;
	if (unlikely(!t || !t->root || !u || len <= 0))
		return -1;

	ut_dbg(UT_DEBUG_TRACE, "[insert] for(len:%d) %s start\n", len, u);

	st = t->root;
	if (memcmp(u, st->str, 1) != 0)
		return -1;
	if (len <= 1) {
		return 0;
	}
	st = ucb_subtree_search(st, u+1, len-1, &parent, &left); 
	if (st) {
		ut_dbg(UT_DEBUG_INFO, "already exist for(len:%d) %s failed\n", len, u);
		return 1;
	}

	if (!parent) {
		ut_dbg(UT_DEBUG_ERR,"insert failed, no parent for len:%d, str:%s\n", len, u);
		return -1;
	}
	ut_dbg(UT_DEBUG_TRACE, "[insert] for(len:%d) %s to parent:%d:%s\n", 
		len, u, parent->str_len, parent->str);

	ret = ucb_subtree_insert(parent, u + len - left, left);
	if (ret) {
		ut_dbg(UT_DEBUG_ERR,"insert failed for (len:%d) url:%s\n", len, u);
		return -1;
	}
	return 0;
}

#if 0
/*
 * Return: 1, exist; 0 success; -1, error
 */
static inline int __ucb_subtree_insert(ucb_subtree *t, const char *u, int u_len)
{
	const unsigned char*const ubytes= (void*)u;
	const size_t ulen= u_len;
	unsigned char* p= t->root;
	ucb_subtree *leaf = NULL;

	if(!p){
		leaf = ucb_subtree_create(t, u , ulen);
		if(leaf) {
			ut_dbg(UT_DEBUG_ERR,"url subtree create failed\n");
			return -1;
		}
		return 0;
	}

	while(1&(intptr_t)p){
		ucb_subnode*q= (void*)(p-1);

		unsigned char c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		const int direction= (1+(q->otherbits|c))>>8;

		p= q->child[direction];
	}

	uint32 newbyte;
	uint32 newotherbits;

	for(newbyte= 0;newbyte<ulen;++newbyte){
		if(p[newbyte]!=ubytes[newbyte]){
			newotherbits= p[newbyte]^ubytes[newbyte];
			goto different_byte_found;
		}
	}

	if(p[newbyte]!=0){
		newotherbits= p[newbyte];
		goto different_byte_found;
	}
	return 1;

different_byte_found:
	/* get the highest different bit*/
	newotherbits|= newotherbits>>1;
	newotherbits|= newotherbits>>2;
	newotherbits|= newotherbits>>4;
	newotherbits= (newotherbits&~(newotherbits>>1))^255;
	/* compute direction*/
	unsigned char c= p[newbyte];
	int newdirection= (1+(newotherbits|c))>>8;

	ucb_subnode*newnode;
	if(posix_memalign((void**)&newnode,sizeof(void*),sizeof(ucb_subnode)))
		return 0;

	leaf = ucb_subtree_create(t, u , ulen);
	if(leaf) {
		ut_dbg(UT_DEBUG_ERR,"url subtree create failed\n");
		UT_FREE(newnode);
		return -1;
	}

	newnode->byte= newbyte;
	newnode->otherbits= newotherbits;
	newnode->child[1-newdirection]= leaf;

	void**wherep= &t->root;
	for(;;){
		unsigned char*p= *wherep;
		if(!(1&(intptr_t)p))break;
		ucb_subnode*q= (void*)(p-1);
		if(q->byte> newbyte)break;
		if(q->byte==newbyte&&q->otherbits> newotherbits)break;
		unsigned char c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		const int direction= (1+(q->otherbits|c))>>8;
		wherep= q->child+direction;
	}

	newnode->child[newdirection]= *wherep;
	*wherep= (void*)(1+(char*)newnode);

	return 0;
}
#endif

static inline int __ucb_subtree_delete(ucb_subtree *t, const char *u, int u_len)
{
	const unsigned char *ubytes= (void*)u;
	const size_t ulen = u_len;
	unsigned char *p = t->root;
	void **wherep = &t->root;
	void **whereq = 0;
	ucb_subnode *q = 0;
	int direction = 0;

	if(!p)
		return 0;

	while(1&(intptr_t)p){
		whereq= wherep;
		q= (void*)(p-1);
		unsigned char c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		direction= (1+(q->otherbits|c))>>8;
		wherep= q->child+direction;
		p= *wherep;
	}

	if(0!=strcmp(u,(const char*)p))return 0;
	free(p);

	if(!whereq){
		t->root= 0;
		return 1;
	}

	*whereq= q->child[1-direction];
	free(q);

	return 1;
}

static inline void
traverse(void*top){
	unsigned char *p= top;

	if(1&(intptr_t)p){
		ucb_subnode*q= (void*)(p-1);
		traverse(q->child[0]);
		traverse(q->child[1]);
		free(q);
	}else{
		free(p);
	}
}

static inline void ucb_subtree_clear(ucb_subtree *t)
{
	if(t->root)traverse(t->root);
	t->root= NULL;

}

#endif
