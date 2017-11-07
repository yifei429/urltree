/*
 *
 *
 *
 */


#ifndef __H_URLTREE_GENERALIZE_H__
#define __H_URLTREE_GENERALIZE_H__

#include <stdio.h>
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

	utstr_t new_url;
	utstr_t param_name;
	utstr_t param_value;
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


#define URLGNZ_BACKREF_SYMBOL "$"
static inline int urlgnz_back_refer(char *des, int sz, const char *src, \
        char back_refer_list[][MAX_URLGNZ_BACKREF_LEN], int back_ref_cnt)
{
        char *find = NULL, *befor_find = NULL;
        int revert_no = 0;
        if (des == NULL  || src == NULL)
                return -1;

        memset(des, 0, sz);

        find = strstr(src, URLGNZ_BACKREF_SYMBOL);
        if (!find) {
                strncat(des, src, sz -1);
		return 0;
        }

        while (find) {
                if (strlen(find) <= 1) {
                        strncat(des, src, sz-strlen(des)-1);
                        find = NULL;
                        continue;
                }
                if (find[1] < '0' || find[1] > '9') {
                        find = strstr(find+1, URLGNZ_BACKREF_SYMBOL);
                        continue;
                }

                if (find-src > 0) {
                        befor_find = find-1;
                        if (befor_find[0] == '\\') {
                                if (sz-strlen(des) > befor_find-src) {
                                        strncat(des, src, befor_find-src);
                                        src = find;
                                        ut_dbg(UT_DEBUG_TRACE, "\\src is %s\n", src);
                                }
                                find = strstr(find+1, URLGNZ_BACKREF_SYMBOL);
                                continue;
                        }
                }

                revert_no = find[1] - '0';
                if (revert_no > 9 || revert_no < 0) {
                        find = NULL;
                        break;
                }

                if (revert_no >= back_ref_cnt) {
                        src = find + 2;
                        find = NULL;
                        strncat(des, src, sz-strlen(des) > strlen(src) ? \
                                strlen(src) : sz-strlen(des)-1);
                        break;
                }

                if (sz-strlen(des) <= (find-src)+strlen(back_refer_list[revert_no])) {
                        strncat(des, src, sz-strlen(des) > find-src ? \
                                find-src : sz-strlen(des)-1);
                        strncat(des, back_refer_list[revert_no], \
                                sz-strlen(des)-1);

                        find = NULL;
                        break;
                }

                strncat(des, src, sz-strlen(des) > find -src ? find -src:sz-strlen(des)-1);
                ut_dbg(UT_DEBUG_TRACE, "The des: %s \n", des);

                strncat(des, back_refer_list[revert_no], sz-strlen(des)-1 );
                ut_dbg(UT_DEBUG_TRACE, "The des2: %s \n", des);

                src = find + 2;

                find = strstr(src, URLGNZ_BACKREF_SYMBOL);

                if (find == NULL) {
                        strncat(des, src, sz-strlen(des)-1);
                }
        }

	return 0;
}


static inline int urlgnz_add_param(char *name, char *value, urlgnz_result *result)
{
        int i = 0;
	urlgnz_param *param, *next, *newparam;

        if (!name || !value || !result)
                return -1;
        if (strlen(name) == 0)
                return -1;

	if (result->param_count >= MAX_URLGNZ_PARAM_COUNT)
		return -1;

	param = result->param;
	while(param) {
		next = param->next;
                if (strcasecmp(param->name.str, name) == 0) {
			utstr_free(&param->value);
			if (utstr_create_bysrc(&param->value, 
				value, strlen(value))) {
				ut_dbg(UT_DEBUG_ERR, "create param value failed\n");
				return -1;
			}
                        return 0;
                }
		if(next) {
			param = next;
		} else {
			break;
		}
	}

	newparam = UT_MALLOC(sizeof(urlgnz_param));
	if (!newparam) {
		ut_dbg(UT_DEBUG_ERR, "create param failed\n");
		return -1;
	}
	memset(newparam, 0x0, sizeof(urlgnz_param));
	if (utstr_create_bysrc(&param->name, name, strlen(name))) {
		ut_dbg(UT_DEBUG_ERR, "create newparam name failed\n");
		goto failed;
	}

	if (utstr_create_bysrc(&param->value, value, strlen(value))) {
		ut_dbg(UT_DEBUG_ERR, "create newparam name failed\n");
		goto failed;
	}
               
	if (param)
		param->next = newparam;
	else 
		result->param = newparam;

	result->param_count++;
	ut_dbg(UT_DEBUG_TRACE, "param count %d\n", result->param_count);
	
	return 0;
failed:
	if (newparam) {
		utstr_free(&newparam->name);
		utstr_free(&newparam->value);
		UT_FREE(newparam);
	}	
	return -1;
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
			UT_FREE(param);
			param = next;
		}
		res->param = NULL;
	}
	
	utstr_free(&res->new_url);
	UT_FREE(res);
	return;
}


/*
 * Return: 1, replaced; 0, unchanged; -1, error;
 *
 */
static inline urlgnz_result* urlgnz_replace(char *src_url, int len, urlgnz_patten_r *rule)
{
	urlgnz_result *result;
	int ovector[MAX_URLGNZ_BACKREF_COUNT *2 + 2] = {0}, pcre_ret = 0;
	int i, k;
	int back_ref_cnt = 0, back_ref_len = 0;
	char tmp_para_value[MAX_URLGNZ_PARMVALUELEN];

	char back_ref_list[MAX_URLGNZ_BACKREF_COUNT][MAX_URLGNZ_BACKREF_LEN];
	if (unlikely(!src_url || len <= 0 || !rule)) 
		return NULL;

	ovector[0] = -1;
	ovector[1] = 0;
	pcre_ret = pcre_exec(rule->exp, rule->extra_exp, \
		src_url, strlen(src_url), 0, 0, ovector, 300);
	ut_dbg(UT_DEBUG_TRACE, "rule %s pcre result %d for url %s\n", 
		rule->url_reg_exp, pcre_ret);

	if (pcre_ret <= 0 || ovector[0] < 0 || ovector[1] <=0)
		return NULL;
	
	for(i = 0; i < pcre_ret; i++) {
		if(back_ref_cnt > MAX_URLGNZ_BACKREF_COUNT
			|| i > MAX_URLGNZ_BACKREF_COUNT) { 
			ut_dbg(UT_DEBUG_WARNING, "too many back ref for url replace, max %d, i:%d, cnt:%d\n", 
				MAX_URLGNZ_BACKREF_COUNT, i, back_ref_cnt);
			break;
		}
		
		k = i*2;
		if (ovector[k+1]-ovector[k] >= MAX_URLGNZ_BACKREF_LEN) {
			back_ref_len = MAX_URLGNZ_BACKREF_LEN - 1;
		} else {
			back_ref_len = ovector[k+1]-ovector[k];
		}
		memcpy(back_ref_list[back_ref_cnt], 
			src_url+ovector[k], back_ref_len);
		back_ref_list[back_ref_cnt][back_ref_len] = '\0';
		back_ref_cnt++;
	}

	result = UT_MALLOC(sizeof(urlgnz_result));
	if (!result) {
		ut_dbg(UT_DEBUG_ERR, "malloc for url replace reuslt failed\n");
		goto failed;
	}
	memset(result, 0x0, sizeof(urlgnz_result));

        /* form the result strings, url first */
        if (rule->new_url.len > 0) {
		if (utstr_create(&result->new_url, MAX_URLGNZ_URLLEN)) {
			ut_dbg(UT_DEBUG_ERR, "create new_url failed\n");
			goto failed;
		}
                urlgnz_back_refer(result->new_url.str, result->new_url.len, \
                        rule->new_url.str, back_ref_list, 10);
		result->new_url.len = strlen(result->new_url.str);
        }
        ut_dbg(UT_DEBUG_TRACE, "the new url is %s\n", result->new_url.str);

        /* form the parameter */
        if (rule->param_name.len > 0) {
                urlgnz_back_refer(tmp_para_value, sizeof(tmp_para_value), \
                        rule->param_value.str, back_ref_list, 10);

                urlgnz_add_param(rule->param_name.str, tmp_para_value, result);
                ut_dbg(UT_DEBUG_TRACE, "add param:%s,%s\n", rule->param_name, tmp_para_value);
        }

	return result;
failed:
	if (result)
		urlgnz_free(result);
	return NULL;
}




#endif
