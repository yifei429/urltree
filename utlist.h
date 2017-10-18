/*
 *	Date: 2017-10-18
 *
 *
 *
 */

#ifndef __H_UTLIST_H__
#define __H_UTLIST_H__

typedef struct utlist {
	struct utlist	*p;	/* previous element */
	struct utlist	*n;	/* next element */
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

#endif /* end of __H_UTLIST_H__  */


