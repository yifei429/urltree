/*
 *
 *
 *
 */


#ifndef __H_URLTREE_GENERALIZE_H__
#define __H_URLTREE_GENERALIZE_H__


#include <pcre.h>
#include "utlist.h"

/* url generalize */
typedef struct  _urlgnz_pattern_rule_t{
	utlist_t 	list;	/* must be first */
#define URL_PREDEFINED_JSP 	1
#define URL_PREDEFINED_OWA2K3	2
#define URL_CUSTOM 		3
	unsigned char type;
	utstr_t	url_reg_exp;
	pcre *exp;
	pcre_extra *extra_exp;

	utstr_t *newurl;
	utstr_t *extadd_param_name;
	utstr_t *extadd_param_value;
} urlgnz_patten_r;

typedef struct _urlgnz_patten_policy_t {
	utlist_head_t *head;
} urlgnz_patten_p;

typedef struct _urlgnz_param_t {
	struct _urlgnz_param_t *next;
	utstr_t name; 
	utstr_t value; 
} urlgnz_param;

typedef struct _urlgnz_result_t {
	utstr_t	new_url;
	urlgnz_param *param;
	int param_count;
} urlgnz_result;

/*
 * Return: 1, replaced; 0, unchanged; -1, error;
 *
 */
static inline urlgnz_result* urlgnz_replace(char *url, int len, urlgnz_patten_p *policy)
{
	urlgnz_result *result = NULL;
	if (unlikely(!url || len <= 0 || !policy)) 
		return NULL;


	
	return result;
}

static inline void urlgnz_free(urlgnz_result *res)
{
	urlgnz_param *param, *next;
	if (!res)
		return;
	
	if (res->param) {
		param = res->param;
		while(param) {
			next = param->next;
			utstr_free(&param->name);
			utstr_free(&param->value);
			param = next;
		}
		res->param = NULL;
	}
	
	utstr_free(&res->new_url);
	UT_FREE(res);
	return;
}









#endif
