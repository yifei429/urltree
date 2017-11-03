/*
 *	Date: 2017-10-18
 *
 *
 *
 */

#ifndef __H_UTLIST_H__
#define __H_UTLIST_H__


#include <assert.h>

typedef struct utlist {
	struct utlist	*n;	/* must be first as ulist_head_t */
	struct utlist	*p;
} utlist_t;

typedef struct utlist_head_t {
	struct utlist 	*n;	/* must be same as utlist->n*/
	int 		cnt;
} utlist_head_t;

#define	UTLIST_HINIT(h)		({(h)->n = NULL; (h)->cnt = 0;})
#define	UTLIST_IS_HEMPTY(h)	((h)->cnt == 0)
#define UTLIST_HLEN(h)		((h)->cnt)

#define	UTLIST_EINIT(l)		((l)->p = (l)->n = NULL)
#define	UTLIST_ELEM(l, t, m)	((t)(((void *)(l)) - ((void *)&((t)0)->m)))
#define	UTLIST_IS_UNLINK(l)	((l)->n == NULL && (l)->p == NULL)

/* get first element in list */
#define	UTLIST_GET_FIRST(lh, t, m)			\
	(((lh)->n == NULL) ? NULL : UTLIST_ELEM((lh)->n, t, m))

#define	UTLIST_ADD(lh, l)				\
	({						\
	 	(l)->n = (lh)->n;			\
	 	(l)->p = ((utlist_t *)(lh));		\
		if ((lh)->n != NULL)			\
	 		(l)->n->p = (lh)->n = (l);	\
		(lh)->n = (l);				\
	 	(lh)->cnt++;				\
	})

#define	UTLIST_DEL(lh, l)					\
	({ 						\
		if ((l)->n)				\
	 		(l)->n->p = (l)->p;		\
	 	(l)->p->n = (l)->n;			\
	 	UTLIST_EINIT(l);				\
	 	(lh)->cnt--;				\
	 })



#define MAX_NODES	10000
#define ut_log(fmt, args...) printf("[LOG]<%s:%d> "fmt, __FILE__, __LINE__, ##args)
//#define ut_err(fmt, args...) printf("<%s:%d> "fmt, __FILE__, __LINE__, ##args)

#define MAX_SEARCH_TIME 1

#define UT_DEBUG_LEVEL 		3

#define UT_DEBUG_ERR 		1
#define UT_DEBUG_ALERT		2
#define UT_DEBUG_WARNING	3
#define UT_DEBUG_INFO		4
#define UT_DEBUG_TRACE		5
#define UT_DEBUG_MAX		6
static inline const char *ut_dbg_level(int level)
{	
	static char *levels[UT_DEBUG_MAX] = {
		[UT_DEBUG_ERR] 		= "error",
		[UT_DEBUG_ALERT] 	= "alert",
		[UT_DEBUG_WARNING] 	= "warning",
		[UT_DEBUG_INFO] 	= "info",
		[UT_DEBUG_TRACE] 	= "trace",
	};
	return levels[level];
}
#define ut_dbg(level, fmt, args...) 	\
	if (UT_DEBUG_LEVEL >=level)	\
		printf("[%10s]<%s:%d> "fmt, ut_dbg_level(level), __FILE__, __LINE__, ##args)

#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif


#define UT_MALLOC(x) malloc(x)
#define UT_FREE(x) free(x)
#define UT_MALLOC_ALIGN(x, align, size) posix_memalign((x),(align), (size))

static inline char *ut_str_slash(char *str, int len)
{
	assert(len >=0);
	while(len > 0) {
		if (*str == 47)
			return str + 1;
		str++;
		len--;
	}
	return str + len;
}



#endif /* end of __H_UTLIST_H__  */


