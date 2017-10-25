/*
 *
 *
 *
 **/



#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "url_cbtree.h"

/* ============ for memory ============== */
/* or use malloc_stats(), man malloc_stats for detail */
#include <malloc.h>
void
display_mallinfo(char *msg, struct mallinfo *s)
{
	struct mallinfo mi;
        mi = mallinfo();
	*s = mi;

	return;
        if (msg) {
                printf("===%s===\n",msg);
        }
        printf("\tTotal non-mmapped bytes (arena):       %d\n", mi.arena);
        printf("\t# of free chunks (ordblks):            %d\n", mi.ordblks);
        printf("\t# of free fastbin blocks (smblks):     %d\n", mi.smblks);
        printf("\t# of mapped regions (hblks):           %d\n", mi.hblks);
        printf("\tBytes in mapped regions (hblkhd):      %d\n", mi.hblkhd);
        printf("\tMax. total allocated space (usmblks):  %d\n", mi.usmblks);
        printf("\tFree bytes held in fastbins (fsmblks): %d\n", mi.fsmblks);
        printf("\tTotal allocated space (uordblks):      %d\n", mi.uordblks);
        printf("\tTotal free space (fordblks):           %d\n", mi.fordblks);
        printf("\tTopmost releasable block (keepcost):   %d\n", mi.keepcost);
}
void display_usedmem(struct mallinfo *start, struct mallinfo *end)
{
	printf("non-mmapped bytes: 		%d\n", end->arena - start->arena);	
	printf("mapped regions:			%d\n", end->hblks- start -> hblks);
	printf("allocated space:		%d\n", end->uordblks - start->uordblks);
	printf("freed sapce:			%d\n", end->fordblks - start->fordblks);
}


struct urlpath {
	struct urlpath *next;
	int str_len;
	char *str;
};

void url_path_free(struct urlpath *head)
{
	printf("url path free\n");
	struct urlpath *next;
	while(head) {
		next = head->next;
		if (head->str)
			free(head->str);
		free(head);
		head = next;
	}
	return;
}

void url_path_dump(struct urlpath *head)
{
	struct urlpath *bak = head;
	int i = 1;
	while (bak) {
		printf("%d:%d:%s\n", i,bak->str_len, bak->str);
		bak = bak->next;
		i++;
	}

}

struct urlpath *read_url_path(char *filename, int *cnt)
{
	FILE *f;
	struct urlpath *head = NULL, *prev = NULL, *path;	
	char buf[512];
	int len = 0;
	f  = fopen(filename, "r"); 
	if (!f) {
		printf("open %s failed\n", filename);
		return NULL;
	}

	*cnt = 0;
	while (fgets(buf, sizeof(buf), f)) {
		len = strlen(buf);
		if (buf[len -1] == '\n') {
			buf[len -1 ] = '\0';
			len = len - 1;
		}
		path = malloc(sizeof(struct urlpath));
		if (!path) {
			printf("malloc urlpath failed\n");
			return NULL;
		}
		memset(path, 0x0, sizeof(struct urlpath));
		path->str = malloc(len + 1);
		if (!path->str) {
			printf("malloc str %d failed\n", len);
			return NULL;
		}
		snprintf(path->str, len + 1, "%s", buf);
		path->str_len = len;

		*cnt = *cnt +1;
		if (!head)	
			head = path;
		if (prev)
			prev->next = path;
		prev = path;
	}
	return head;
}

ucb_tree *init_cbtree(struct urlpath *head)
{
	ucb_tree *t = NULL;
	struct  timeval  start, end;

	gettimeofday(&start,NULL);
	t = ucb_tree_create();
	if (!t) {
		ut_err("create ut tree failed\n");
		return NULL;
	}
	while(head) {
		ucbtree_insert(t, head->str, head->str_len);
		head = head->next;
	}

	gettimeofday(&end,NULL);
	printf("create tree timer = %ld us\n", 1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec);
	return t;
}

int ucb_tree_search(ucb_tree *t, struct urlpath *head)
{
	int ret = 0;
	struct urlpath *bak = head;
	int i = 0, max = 10000;
	long total_search = 0, total_time  =0;;
	struct  timeval  start, end;

	gettimeofday(&start,NULL);
	for( i = 0; i < max; i++) {
		head = bak;
		while (head) {
			ret = ucbtree_search(t, head->str, head->str_len);
			if (ret <= 0) {
				printf("unknown url %s\n", head->str);
			} else {
				//printf("find url level:%d, url:%s\n", ret, head->str);
			}
			head = head->next;
			total_search++;
		}
	}
	gettimeofday(&end,NULL);
	total_time = 1000000 * (end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec);
	printf("timer = %ld us, search cout:%ld, search:%ld times/persecond\n", 
		total_time, total_search, (long)(total_search * 1000000) /(total_time));

	return 0;
}

extern int max_depth;
int main(int argc, char **argv)
{
	struct urlpath *head = NULL, *bak;	
	int cnt = 0;
	char buf[512];
	int i = 1;
	ucb_tree *tree = NULL;
	struct mallinfo mstart, mend;

	if (argc != 2) {
		printf("usage: ./main website.txt\n");
		return -1;
	}

	head = read_url_path(argv[1], &cnt);
	if (!head || cnt <= 0) {
		printf("read web site failed %d\n", cnt);
		return -1;
	}
	printf("total url cnt :%d\n", cnt);

	
	printf("======> memory start\n");
	printf("==> memory stats\n");
	malloc_stats();
	display_mallinfo(NULL, &mstart);
	tree = init_cbtree(head);
	if (!tree) {
		printf("init tree failed\n");
		goto out;
	}

	display_mallinfo(NULL, &mend);
	printf("======> memory end\n");
	display_usedmem(&mstart, &mend);
	printf("==> memory stats\n");
	malloc_stats();

	//url_path_dump(head);
	//ucb_tree_dump(tree);
	ucb_tree_search(tree, head);
	
	printf("max depth is :%d\n", max_depth);
out:
	if (tree)
		ucb_tree_release(tree);
	if (head)
		url_path_free(head);
	return 0;
}
