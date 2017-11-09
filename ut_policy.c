/*
 * Date:
 *
 */


#include <pthread.h>

#include "urltree.h"
#include "ut_policy.h"


ptimer_tree_t __dbtimer; 
ptimer_tree_t *dbtimer = NULL; 
static int utp_test = 0;

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
	
	ut_dbg(UT_DEBUG_INFO, "release db message array for tree %s\n", msgs->tree_tname);
	pthread_rwlock_unlock(&msgs->lock);
	UT_FREE(msgs);
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

	ut_dbg(UT_DEBUG_INFO, "create db message array for tree %s\n", tablename);

	return msgs;
failed:
	if (msgs) {
		utp_msgs_release(msgs);
	}
	return NULL;
}

static inline int _utp_refresh_db(utp_url_dbmsg *msg, int db)
{
	/* run sql command to change the database */
	ut_dbg(UT_DEBUG_TRACE, "[db] update for node %s\n", msg->node->str);
	return 0;
}

/* flush to db frequency by tree node */
int utp_refresh_db(void *args)
{
	utp_url_dbmsg *m, *next;
	int i = 0;
	utp_msgs *msgs = args;

	if (!msgs)
		return -1;

	ut_dbg(UT_DEBUG_INFO, "[db] update for tree %s\n", msgs->tree_tname);
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


int utp_add_msg(utp_msgs *msgs, ut_node *node, int act, int total_cnt)
{
	utp_url_dbmsg *m;
	int timeout = 10;
	if (unlikely(!msgs || !node))
		return -1;

	if(utp_test)
		return 0;

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
	
	if (msgs->cnt == 1) {
		if (total_cnt > 1000) {
			timeout = 300;
		} else if (total_cnt > 500) {
			timeout = 180;
		} else if (total_cnt > 100) {
			timeout = 60;
		} else {
			timeout = 10;
		}
		if (ptimer_is_active(dbtimer, &msgs->timer)) {
			ut_dbg(UT_DEBUG_ERR, "already in timer tree");
			goto out;
		}
		ptimer_add(dbtimer, &msgs->timer, utp_refresh_db, timeout, msgs);
		ut_dbg(UT_DEBUG_INFO, "[db] add timer with db node %s for tree %s\n", 
			node->str, msgs->tree_tname);
	} else {
		if (msgs->cnt > UTP_MSGS_MAX_CNT * 7/10) {
			timeout = 0;
			ptimer_del(dbtimer, &msgs->timer);
			ptimer_add(dbtimer, &msgs->timer, utp_refresh_db, timeout, msgs);
			ut_dbg(UT_DEBUG_WARNING, "[db] start timer now(cnt:%d) with db node %s for tree %s\n", 
				msgs->cnt, node->str, msgs->tree_tname);
		}
	}
	
	ut_dbg(UT_DEBUG_TRACE, "[db] add db node %s for tree %s\n", node->str, msgs->tree_tname);
out:
	pthread_rwlock_unlock(&msgs->lock);
	return 0;
}

int utp_init(int test)
{
	dbtimer = &__dbtimer;
	utp_test = test;
	return ptimer_init(dbtimer, 1);	
}

void utp_release()
{
	return;
}

void utp_timeout()
{
	ptimer_timeout(dbtimer);
}



