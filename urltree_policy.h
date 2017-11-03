/*
 *
 *
 *
 */

#ifndef __H_URLTREE_POLICY_H__
#define __H_URLTREE_POLICY_H__

/* url filter; lots of urls need not to learn, such as jpg, css and so on */
/* 
 * Return:1, need to learn; 0, don't learn; -1, error;
 **/
static inline int utp_filter(char *url, int len)
{
	return 0;
}



/* url patten policy */

typedef struct  _url_patten_t{
	int type;
} url_patten;







/* record each url by ip, session, or tcp */











/* flush to db frequency by tree node */











#endif
