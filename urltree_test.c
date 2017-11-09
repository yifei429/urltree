/*
 *
 *
 *
 **/



#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "urltree.h"

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
	fclose(f);
	return head;
}

ut_root *insert_tree(ut_root *root, struct urlpath *head)
{
	struct  timeval  start, end;
	ut_node *node;

	gettimeofday(&start,NULL);
	if (!root) {
		ut_dbg(UT_DEBUG_ERR,"create ut tree failed\n");
		return NULL;
	}
	while(head) {
		node = ut_insert(root, head->str, head->str_len);
		if (node)
			ut_node_put(node);
		else 
			printf("insert node failed\n");
		head = head->next;
	}

	gettimeofday(&end,NULL);
	printf("create tree timer = %ld us\n", 1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec);
	return root;
}

int ut_tree_search(ut_root *root, struct urlpath *head)
{
	int ret = 0;
	struct urlpath *bak = head;
	int i = 0, max = MAX_SEARCH_TIME;
	long total_search = 0, total_time  =0;;
	struct  timeval  start, end;
	ut_node *node;

	gettimeofday(&start,NULL);
	for( i = 0; i < max; i++) {
		head = bak;
		while (head) {
			node = ut_search(root, head->str, head->str_len);
			if (!node) {
				printf("unknown url %s\n", head->str);
			} else {
				ut_node_put(node);
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

int ut_tree_delete(ut_root *root, struct urlpath *head)
{
	int ret = 0;
	struct urlpath *bak = head;
	int i = 0, max = 1;

	for( i = 0; i < max; i++) {
		head = bak;
		while (head) {
			ret = ut_delete(root, head->str, head->str_len);
			if (ret) {
				printf("unknown url %s\n", head->str);
			} else {
				//printf("find url level:%d, url:%s\n", ret, head->str);
			}
			head = head->next;
		}
	}
	ret = ut_delete(root, bak->str, bak->str_len);
	if (ret) {
		printf("test for 2nd delete, already delete.\n");
	} else {
		printf("error, delete twice\n");
	}
	ret = ut_delete(root, "/", 1);
	if (ret) {
		printf("test for 2nd delete, already delete.\n");
	} 

	return 0;
}

int test_type = 0;
char web_filename[256];
int _parse_opt(int argc, char **argv)
{
        char c;
        //const char optstr[] = ":d:h:p";
        const char optstr[] = "t:f:h";
	char *ptr;

        opterr = 0;
        while ( (c = getopt(argc, argv, optstr)) != -1) {
                switch (c) {
                case 't':
			test_type = atoi(optarg);
			printf("test type:%d\n", test_type);
                        break;
		case 'f':
			sprintf(web_filename, "%s", optarg);
			printf("test web_filename:%s\n", web_filename);
			break;
                case 'h':
			printf("Usage: ./main -t <number>\n,"
				"1, tree memory; 2, for search;"
				"3, for db;\n");
                        exit(0);
                case '?':
                        printf("unknow option %c\n", optopt);
                        return -1;
                }
        }
        if (optind != argc)
                return -1;

        return 0;
}

void *timeout_thread(void *arg)
{
	while(1) {
		usleep(10);
		ut_timeout();
	}

	return NULL;
}

int main(int argc, char **argv)
{
	struct urlpath *head = NULL, *bak;	
	int cnt = 0;
	char buf[512];
	int i = 1;
	ut_root *root = NULL;
	struct mallinfo mstart, mend;
	pthread_t thread_id;

	if (_parse_opt(argc, argv)) {
		printf("wrong option\n");
		return -1;
	}
	if (strlen(web_filename) <= 0) {
		printf("missinge filename for websiete\n");
		return -1;
	}


	head = read_url_path(web_filename, &cnt);
	if (!head || cnt <= 0) {
		printf("read web site failed %d\n", cnt);
		return -1;
	}
	printf("total url cnt :%d\n", cnt);
	switch(test_type){
	case 0:
		ut_global_init(1);
		root = ut_tree_create("test_website");
		if (!root) {
			printf("init tree failed\n");
			goto out;
		}

		printf("======> memory start\n");
		display_mallinfo(NULL, &mstart);
		printf("[stats]\n");
		malloc_stats();

		insert_tree(root, head);

		//url_path_dump(head);
		//ut_tree_dump(root);
		display_mallinfo(NULL, &mend);
		printf("======> memory end\n");
		printf("[stats]\n");
		malloc_stats();
		printf("[mallinfo]\n");
		display_usedmem(&mstart, &mend);

		printf("sizeof(utnode_head_t) is :%ld, bucket:%ld\n", 
			sizeof(utnode_head_t), sizeof(utnode_head_t) * UTHASH_MAX_BUCKETS );

		printf("root count:%d\n", root->total_node);
#ifdef UT_HASH_CACHE
		printf("hash count:%d\n", root->hash->node_cnt);
#endif
		ut_tree_delete(root, head);
		ut_tree_dump(root);

		break;
	case 2:
		ut_global_init(1);
		root = ut_tree_create("test_website");
		if (!root) {
			printf("init tree failed\n");
			goto out;
		}

		insert_tree(root, head);
		ut_tree_search(root, head);

#if 0
		printf("=============>insert again\n");
		insert_tree(root, head);
		ut_tree_dump(root);
		printf("=============>delete again\n");
		ut_tree_delete(root, head);
		ut_tree_dump(root);
#endif

		break;
	case 3:
		ut_global_init(0);
		if (pthread_create(&thread_id, NULL, timeout_thread, NULL)) {
			printf("start thread for db timer failed\n");
			return -1;
		}
		sleep(1);
		root = ut_tree_create("test_website");
		if (!root) {
			printf("init tree failed\n");
			goto out;
		}

		insert_tree(root, head);
		while (1) {
			usleep(10);
		}
		break;
	default:
		break;
	}
out:
	if (root)
		ut_tree_release(root);
	if (head)
		url_path_free(head);

	ut_global_free();
	return 0;
}



