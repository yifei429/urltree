/*
 *
 *
 *
 */

#ifndef __H_URLTREE_POLICY_H__
#define __H_URLTREE_POLICY_H__

#include "utlist.h"
#include "urltree.h"

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










typedef struct _utp_db_urlmsg_t {
	
} utp_db_urlmsg;
/* flush to db frequency by tree node */











#endif
