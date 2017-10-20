/*
 *
 *
 *
 */

#ifndef __H_URL_CBTREE_H__
#define __H_URL_CBTREE_H__

typedef struct {
  void *root;
} ucb_subtree;

typedef struct {
	void *child[2];
	uint32 byte;
	uint8 otherbits;
} ucb_subnode;

int ucb_subtree_contains(ucb_subtree *t, const char *u);
int ucb_subtree_insert(ucb_subtree *t, const char *u);
int ucb_subtree_delete(ucb_subtree *t, const char *u);
void ucb_subtree_clear(ucb_subtree *t);
int ucb_subtree_allprefixed(ucb_subtree *t, const char *prefix,
	int (*handle) (const char *, void *), void *arg);


int
critbit0_contains(critbit0_tree*t,const char*u){
	const uint8*ubytes= (void*)u;
	const size_t ulen= strlen(u);
	uint8*p= t->root;

	/*4:*/
#line 86 "./critbit.w"

	if(!p)return 0;

	/*:4*/
#line 76 "./critbit.w"

	/*5:*/
#line 110 "./critbit.w"

	while(1&(intptr_t)p){
		critbit0_node*q= (void*)(p-1);
		/*6:*/
#line 136 "./critbit.w"

		uint8 c= 0;
		if(q->byte<ulen)c= ubytes[q->byte];
		const int direction= (1+(q->otherbits|c))>>8;

		/*:6*/
#line 113 "./critbit.w"

		p= q->child[direction];
	}

	/*:5*/
#line 77 "./critbit.w"

	/*7:*/
#line 152 "./critbit.w"

	return 0==strcmp(u,(const char*)p);

	/*:7*/
#line 78 "./critbit.w"

}




#endif
