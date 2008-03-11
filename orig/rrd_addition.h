/* -*- C -*-
 * file:   rrd_addition.h
 * date:   $Date: 2008/03/11 22:47:07 $
 * init:   2005-07-26
 * vers:   $Version$
 * auth:   $Author: dbach $
 * -----
 * 
 * Support file to add rrd_info() to rrd.h
 * 
 */
#ifndef __RRD_ADDITION_H
#define __RRD_ADDITION_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* rrd info interface */
enum info_type   { RD_I_VAL=0,
	       RD_I_CNT,
	       RD_I_STR, 
		   RD_I_INT };

typedef union infoval { 
    unsigned long u_cnt; 
    rrd_value_t   u_val;
    char         *u_str;
    int		  u_int;
} infoval;

typedef struct info_t {
    char            *key;
    enum info_type  type;
    union infoval   value;
    struct info_t   *next;
} info_t;

info_t *rrd_info(int, char **);
info_t *rrd_info_r(char *);



#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* __RRD_ADDITION_H */
