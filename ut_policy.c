/*
 * Date:
 *
 */


#include <pthread.h>

#include "urltree.h"
#include "ut_policy.h"


void utp_msgs_release(utp_msgs *msgs)
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

utp_msgs* utp_msgs_create(char *tablename)
{
	utp_msgs *msgs;
	
	if (!tablename)
		return NULL;
	msgs = UT_MALLOC(sizeof(utp_msgs));
	if (!msgs) {
		ut_dbg(UT_DEBUG_ERR,"create msgs failed for tree msgs\n");
		return NULL;
	}
	memset(msgs, 0x0, sizeof(utp_msgs));
	if(pthread_rwlock_init(&msgs->lock, NULL)) {
		ut_dbg(UT_DEBUG_ERR,"pthread init failed for tree msgs\n");
		UT_FREE(msgs);
		return NULL;
	}

	if (strlen(tablename) > sizeof(msgs->tree_tname) -1) {
		ut_dbg(UT_DEBUG_ERR,"too long table name:%s,max:%d\n", tablename, sizeof(msgs->tree_tname) -1);
		goto failed;
	}
	snprintf(msgs->tree_tname, sizeof(msgs->tree_tname),"%s", tablename);

	return msgs;
failed:
	if (msgs) {
		utp_msgs_release(msgs);
	}
	return NULL;
}


int utp_add_msg(utp_msgs *msgs, ut_node *node, int act)
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
	/* run sql command to change the database */
	return 0;
}

/* flush to db frequency by tree node */
int utp_refresh_db(utp_msgs *msgs)
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



