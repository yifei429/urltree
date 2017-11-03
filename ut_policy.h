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



/* url patten policy */
typedef struct  _url_patten_t{
#define URL_PREDEFINED_JSP 	1
#define URL_PREDEFINED_OWA2K3	2
#define URL_CUSTOM 		3
	unsigned char type;
	char *url_reg_exp;
	char *newurl;
	char *extadd_param_name;
	char *extadd_param_value;
} url_patten;







/* record each url by ip, session, or tcp */











/* flush to db frequency by tree node */











#endif
