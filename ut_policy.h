/*
 *
 *
 *
 */

#ifndef __H_URLTREE_POLICY_H__
#define __H_URLTREE_POLICY_H__

#include "utlist.h"

/* url filter; lots of urls need not to learn, such as jpg, css and so on */
/* 
 * Return:1, need to learn; 0, don't learn; -1, error;
 **/
#define URL_TYPE_NA 		0x0000
#define URL_TYPE_CSS 		0x0001
#define URL_TYPE_JS		0x0002
#define URL_TYPE_COMMON_JS	0x0004
#define URL_TYPE_TEXT_JS	0x0008
#define URL_TYPE_MAX		0xffff
static inline int utp_filter(char *url, int len, int type)
{
	if (type & URL_TYPE_MAX)
		return 1;
	return 0;
}




/* record each url by ip, session, or tcp */






struct _ut_node;
typedef enum {
	utp_dbmsg_add = 0x00,
	utp_dbmsg_del = 0x01,
	utp_dbmsg_refresh = 0x02,
} utp_dbmsg_act;
typedef struct _utp_url_dbmsg {
	struct _ut_node *node;
	utp_dbmsg_act act;
	struct _utp_url_dbmsg *next;
} utp_url_dbmsg;

typedef struct _utp_msgs_t {
	char tree_tname[MAX_URLTREE_TABLE_NAME];
	utp_url_dbmsg *head;
	utp_url_dbmsg *tail;
#define UTP_MSGS_MAX_CNT 2000
	int cnt;
	pthread_rwlock_t lock;
} utp_msgs;

utp_msgs* utp_msgs_create(char *tablename);
void utp_msgs_release(utp_msgs *msgs);
int utp_refresh_db(utp_msgs *msgs);

int utp_add_msg(utp_msgs *msgs, struct _ut_node *node, int act);

#if 0
static inline void utp_msgs_release(utp_msgs *msgs)
{
	utp_url_dbmsg *m, *next;
	if (!msgs)
		return;
	
	pthread_rwlock_wrlock(&msgs->lock);
	m = msgs->head;
	while (m) {
		next = m->next;

		ut_node_put(m->node);
		UT_FREE(m);
		m = next;
	}
	
	UT_FREE(msgs);
	pthread_rwlock_unlock(&msgs->lock);
	return;
}

static inline utp_msgs* utp_msgs_create(char *tablename)
{
	utp_msgs *msgs;
	
	if (!tablename)
		return -1;
	msgs = UT_MALLOC(sizeof(utp_msgs));
	if (!msgs) {
		ut_dbg(UT_DEBUG_ERR,"create msgs failed for tree msgs\n");
		return -1;
	}
	memset(msgs, 0x0, sizeof(utp_msgs));
	if(pthread_rwlock_init(&msgs->lock, NULL)) {
		ut_dbg(UT_DEBUG_ERR,"pthread init failed for tree msgs\n");
		UT_FREE(msgs);
		return -1;
	}

	if (strlen(tablename) > sizeof(msgs->tree_tname) -1) {
		ut_dbg(UT_DEBUG_ERR,"too long table name:%s,max:%d\n", tablename, sizeof(msgs->tree_tname) -1);
		return -1;
	}
	snprintf(msgs->tree_tname, sizeof(msgs->tree_tname),"%s", tablename);

	return 0;
failed:
	if (msgs) {
		utp_msgs_release(msgs);
	}
	return -1;
}


static inline int utp_add_msg(utp_msgs *msgs, ut_node *node, int act)
{
	utp_url_dbmsg *m;
	if (unlikely(!msgs || !node))
		return -1;

	pthread_rwlock_wrlock(&msgs->lock);
	if (msgs->cnt >= UTP_MSGS_MAX_CNT) {
		ut_dbg(UT_DEBUG_ERR, "too many db msg, max:%d\n", UTP_MSGS_MAX_CNT);
		goto out;
	}
	
	m = UT_MALLOC(sizeof(utp_url_dbmsg));
	memset(m, 0x0, sizeof(sizeof(utp_url_dbmsg)));

	__ut_node_get(node);
	m->node = node;
	m->act = act;

	if (msgs->head == NULL) {
		msgs->head = msgs->tail = m;
	} else {
		msgs->tail->next = m;
		msgs->tail = m;
	}
	msgs->cnt++;
	
out:
	pthread_rwlock_unlock(&msgs->lock);
	return 0;
}

static inline int _utp_refresh_db(utp_url_dbmsg *msg, int db)
{
	return 0;
}

/* flush to db frequency by tree node */
static inline int utp_refresh_db(utp_msgs *msgs)
{
	utp_url_dbmsg *m, *next;
	int i = 0;
	if (!msgs)
		return -1;

	pthread_rwlock_rdlock(&msgs->lock);
	if (msgs->cnt == 0)
		goto out;

	m = msgs->head;
	while(m) {
		next = m->next; 
		i++;
		
		_utp_refresh_db(m, 0);
		ut_node_put(m->node);
		UT_FREE(m);

		m = next;
	}

	assert(i == msgs->cnt);
	msgs->head = msgs->tail = NULL;
	msgs->cnt = 0;

out:
	pthread_rwlock_unlock(&msgs->lock);
	return 0;
}
#endif










#endif
