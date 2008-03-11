/* -----
 * file:   rubyrrdtool.c
 * date:   $Date: 2008/03/11 22:47:07 $
 * init:   2005-07-26
 * vers:   $Version$
 * auth:   $Author: dbach $
 * -----
 * This is a Ruby extension (binding) for RRDTool
 * 
 *   http://people.ee.ethz.ch/~oetiker/webtools/rrdtool/index.en.html
 * 
 * It is based on the initial work done by Miles Egan <miles@caddr.com>
 * and modified by Mark Probert (probert at acm dot org) to bring 
 * support in line with RRDTool 1.2.x
 * 
 * Released under the MIT Licence
 * 
 */

#include <unistd.h>
#include <stdbool.h>
#include <math.h>   /* for isnan */
#include <ruby.h>
#include <rrd.h>
#include "rrd_addition.h"

/* printf debugging */
#define  R_RRD_DEBUG_OFF 0  /* no debugging   */
#define  R_RRD_DEBUG_SIM 1  /* basic debug    */
#define  R_RRD_DEBUG_DET 2  /* more details   */
#undef   R_RRD_DBG

#ifdef R_RRD_DBG
void _dbug(int level, char *s) {
    if (level <= R_RRD_DBG) {
        printf(" --> %s\n", s);
    }
}
#endif

VALUE cRRDtool;     /* class  */
VALUE rb_eRRDtoolError;

extern int optind;
extern int opterr;

typedef int (*RRDtoolFUNC)(int argc, char ** argv);
typedef int (*RRDtoolOutputFUNC)(int, char **, time_t *, time_t *, unsigned long *,
                                 unsigned long *, char ***, rrd_value_t **);

/*
 * define macros for easy error checking
 */

#define RRD_RAISE  rb_raise(rb_eRRDtoolError, rrd_get_error());

#define RRD_CHECK_ERROR  if (rrd_test_error()) RRD_RAISE

#define NUM_BUF_SZ 80

/*
 * Define a structure to store counted array of strings
 */
typedef struct string_array_t {
    int    len;
    char **strs;
} s_arr;

/*
 * Create a new array of strings based on a Ruby array.  
 * 
 * If name_f value is TRUE, then we need to put the
 * rrdname value as the first element of the array.
 * 
 * If dummy_f is true, then add "dummy" at index 0. 
 * I have no idea why we do this, but it seems to be
 * needed for some reason.  
 * 
 */
static s_arr s_arr_new(VALUE self, bool name_f, bool dummy_f, VALUE strs)
{
    s_arr   a;
    int     i, j; /* index counters */
    VALUE   rrd;  /* name of the RRD we are dealing with */

#ifdef R_RRD_DBG    
    char    buf[NUM_BUF_SZ+1];
#endif
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "s_arr: name_f=[%d] dummy_f=[%d]", name_f, dummy_f);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_DET, buf);
#endif
    
    rrd = rb_iv_get(self, "@rrdname");
    Check_Type(strs, T_ARRAY);
    
    /* set the array length */
    a.len = RARRAY(strs)->len;
    if (name_f)  {  a.len++; }
    if (dummy_f) {  a.len++; }
    a.strs = ALLOC_N(char*, a.len);
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "s_arr: array length set to %d", a.len);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_DET, buf);
#endif
    
    i = 0;
    /* drb: some of the rrd API functions assume the action preceeds the
     * command arguments */
    if (dummy_f) {
        a.strs[i] = strdup("dummy");
        i++;
    }
    
    /* add the rrdname if needed */
    if (name_f) {
        a.strs[i] = strdup(STR2CSTR(rrd));
        i++;
    }
    
    /* now add the strings */
    j = 0;
    while (i < a.len) {
        VALUE v = rb_ary_entry(strs, j);

        switch (TYPE(v)) {
        case T_BIGNUM:
        case T_FIXNUM:
            v = rb_obj_as_string(v);
            /* FALLTHROUGH */
        case T_STRING:
            a.strs[i] = strdup(StringValuePtr(v));
#ifdef R_RRD_DBG    
            snprintf(buf, NUM_BUF_SZ, "s_arr: adding: i=%d val=%s", i, a.strs[i]);
            buf[NUM_BUF_SZ] = 0;
            _dbug(R_RRD_DEBUG_DET, buf);
#endif
            break;
            
        default:
#ifdef R_RRD_DBG    
           snprintf(buf, NUM_BUF_SZ, "s_arr: adding: i=%d type=%d", i, TYPE(v));
           buf[NUM_BUF_SZ] = 0;
           _dbug(R_RRD_DEBUG_DET, buf);
#endif          
            rb_raise(rb_eTypeError, "invalid argument for string array");
            break;
        }
        i++; j++;
    }
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "s_arr: len -> %d", a.len);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
    for (i = 0; i < a.len; i++) {
       snprintf(buf, NUM_BUF_SZ, "s_arr[%d] -> %s", i, a.strs[i]);
       buf[NUM_BUF_SZ] = 0;
       _dbug(R_RRD_DEBUG_SIM, buf);
    }
#endif          
    return a;
}


/*
 * clean up the string array
 */
static void s_arr_del(s_arr a)
{
    int i;
    for (i = 0; i < a.len; i++) {
        free(a.strs[i]);
    }
    free(a.strs);
}


/*
 * add an element to a string array at the start of the array
 */
static bool s_arr_push(char *val, s_arr *sa) {
    char **tmp;
    int    i;
#ifdef R_RRD_DBG    
    char   buf[NUM_BUF_SZ+1];
#endif
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "s_arr_push: n=%d val=%s", sa->len, val);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif          
    
    /* set the array length */
    sa->len += 1;
    tmp = ALLOC_N(char*, sa->len);
    
    i = 0;
    tmp[i++] = strdup(val);
    while(i <= sa->len) {
        if (sa->strs[i-1] != NULL) {
            tmp[i] = strdup(sa->strs[i-1]);
            free(sa->strs[i-1]);
        }
        i++;
    }
    sa->strs = tmp;
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "s_arr_push: len -> %d", sa->len);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
    for (i = 0; i < sa->len; i++) {
       snprintf(buf, NUM_BUF_SZ, "s_arr_add: s_arr[%d] -> %s", i, sa->strs[i]);
       buf[NUM_BUF_SZ] = 0;
       _dbug(R_RRD_DEBUG_SIM, buf);
    }
#endif
    return true;
}


/*
 * reset the internal state
 */
static void reset_rrd_state()
{
    optind = 0; 
    opterr = 0;
    rrd_clear_error();
}


/*
 * Document-method: create
 *
 *   call-seq:
 *     rrd.create(pdp_step, last_up, arg_array) -> [Qnil|Qtrue]
 * 
 * The create function of RRDtool lets you set up new Round Robin 
 * Database (RRD) files. The file is created at its final, full size 
 * and filled with *UNKNOWN* data.
 *
 * create filename [--start|-b start time] [--step|-s step] 
 *        [DS:ds-name:DST:heartbeat:min:max] [RRA:CF:xff:steps:rows]
 * 
 * 
 *  filename
 *     The name of the RRD you want to create. RRD files should end with 
 *     the extension .rrd. However, RRDtool will accept any filename. 
 *
 *  pdp_step (default: 300 seconds)
 *     Specifies the base interval in seconds with which data will be 
 *     fed into the RRD. 
 * 
 *  last_up start time (default: now - 10s)
 *     Specifies the time in seconds since 1970-01-01 UTC when the first 
 *     value should be added to the RRD. RRDtool will not accept any data 
 *     timed before or at the time specified. 
 *
 *  DS:ds-name:DST:dst arguments
 *     A single RRD can accept input from several data sources (DS), for 
 *     example incoming and outgoing traffic on a specific communication line. 
 *     With the DS configuration option you must define some basic properties 
 *     of each data source you want to store in the RRD. 
 * 
 *      ds-name is the name you will use to reference this particular data 
 *      source from an RRD. A ds-name must be 1 to 19 characters long in 
 *      the characters [a-zA-Z0-9_].
 * 
 *      DST defines the Data Source Type. The remaining arguments of a data 
 *      source entry depend on the data source type. For GAUGE, COUNTER, DERIVE, 
 *      and ABSOLUTE the format for a data source entry is:
 * 
 *        DS:ds-name:GAUGE | COUNTER | DERIVE | ABSOLUTE:heartbeat:min:max
 * 
 *      For COMPUTE data sources, the format is:
 * 
 *        DS:ds-name:COMPUTE:rpn-expression
 * 
 * NB:  we use the thread-safe version of the command rrd_create_r()
 */
VALUE rrdtool_create(VALUE self, VALUE ostep, VALUE update, VALUE args)
{
    s_arr         a;        /* varargs in the form of a string array */
    VALUE          rval;     /* our result */
    VALUE          rrd;      /* name of the RRD file to create */
    unsigned long  pdp_step; /* the stepping time interval */
    time_t         last_up;  /* last update time */
#ifdef R_RRD_DBG    
    char   buf[NUM_BUF_SZ+1];
#endif
    int result;
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    
    /* conversion ... */
    pdp_step = NUM2LONG(ostep);
    last_up  = (time_t)NUM2LONG(update);
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "n=[%s] : step=%lu : up=%ld", STR2CSTR(rrd),
             pdp_step, (long int)last_up);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    a = s_arr_new(self, false, false, args);
#ifdef R_RRD_DBG
    _dbug(R_RRD_DEBUG_SIM, "call");
#endif
    
    /* now run the command */
    result = rrd_create_r(STR2CSTR(rrd), pdp_step, last_up, a.len, a.strs);

#ifdef R_RRD_DBG
    _dbug(R_RRD_DEBUG_SIM, "cleanup");
#endif
    s_arr_del(a);

    if (result == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}


/*
 * Document-method: dump
 *
 *   call-seq:
 *     rrd.dump -> [Qnil|Qtrue]
 * 
 * The dump function prints the contents of an RRD in human readable (?) 
 * XML format. This format can be read by rrd_restore. Together they allow 
 * you to transfer your files from one computer architecture to another 
 * as well to manipulate the contents of an RRD file in a somewhat more 
 * convenient manner.
 * 
 * This routine uses th thread-safe version rrd_dump_r()
 * 
 * Note:  This routine is a bit of a mess from a scripting persepective.
 *        rrd_dump_r() doesn't take a filename for the output, it just
 *        spits the XML to stdout.  Redirection from with an extension
 *        seems to be hard.  So, for the moment, we just rely on the 
 *        default behaviour and send in a change request for the fn.
 *
 * For the version that takes 
 *
 * 
 */
#ifdef HAVE_RRD_DUMP_R_2
VALUE rrdtool_dump(VALUE self, VALUE output)
{
    int     ret;    /* result of rrd_dump_r */
    VALUE   rval;   /* our result */
    VALUE   rrd;    /* rrd database filename */
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");

    /* type checking */
    Check_Type(output, T_STRING);
    
    ret = rrd_dump_r(STR2CSTR(rrd), STR2CSTR(output));
    if (ret == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}
#endif

/*
 * Document-method: first
 *
 *   call-seq:
 *     rrd.first(rra_idx) -> [Qnil|BIGNUM(time)]
 *  
 *   The first function returns the UNIX timestamp of the first data sample 
 *   entered into the specified RRA of the RRD file.
 *
 *   The index number of the RRA that is to be examined. If not specified, 
 *   the index defaults to zero. RRA index numbers can be determined through 
 *   rrdtool info. 
 * 
 * NB: this function uses the thread-safe version rrd_first_r()
 * 
 */
VALUE rrdtool_first(VALUE self, VALUE orra_idx)
{
    VALUE   rval;   /* our result */
    VALUE   rrd;    /* rrd database filename */
    int     idx;    /* index in integer form */
    time_t  when;   /* the found value */
#ifdef R_RRD_DBG    
    char   buf[NUM_BUF_SZ+1];
#endif
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    if (orra_idx == Qnil) {  idx = 0; }
    else {
        idx = NUM2INT(orra_idx);
    }
    
    when = rrd_first_r(STR2CSTR(rrd), idx);
    if (when == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = LONG2NUM(when);
    }
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "first: rrd=[%s] : idx=%d : val=%ld",
             STR2CSTR(rrd), idx, when);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    return rval;
}


/*
 * Document-method: last
 *
 *   call-seq:
 *     rrd.last -> [Qnil|BIGNUM(time)]
 *  
 *   The last function returns the UNIX timestamp of the most recent 
 *   update of the RRD.
 * 
 * NB: this function uses the thread-safe version rrd_lasst_r()
 * 
 */
VALUE rrdtool_last(VALUE self)
{
    VALUE   rval;   /* our result */
    VALUE   rrd;    /* rrd database filename */
    time_t  when;   /* the found value */
#ifdef R_RRD_DBG    
    char    buf[NUM_BUF_SZ+1];
#endif
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    
    when = rrd_last_r(STR2CSTR(rrd));
    if (when == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = LONG2NUM(when);
    }
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "last: rrd=[%s] : val=%ld", STR2CSTR(rrd), when);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    return rval;
}


/*
 * Document-method: version
 *
 *   call-seq:
 *     rrd.version -> [Qnil|FLOAT(version_number)]
 *  
 *   Returns the version number of the RRDtool library
 * 
 */
VALUE rrdtool_version(VALUE self)
{
    VALUE   rval;   /* our result */
    double  vers;   /* version number */
#ifdef R_RRD_DBG    
    char    buf[NUM_BUF_SZ+1];
#endif
    
    reset_rrd_state();
    
    vers = rrd_version();
    rval = rb_float_new(vers);
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "version: val=%f", vers);
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    return rval;
}



/*
 * Document-method: update
 * 
 * call-seq:
 *   rrd.update(template, arg_array) -> [Qnil|Qtrue]
 *
 * The update function feeds new data values into an RRD. The data is time aligned 
 *  (interpolated) according to the properties of the RRD to which the data is written.
 *
 *
 * filename
 *   The name of the RRD you want to update. 
 *
 * template
 *   By default, the update function expects its data input in the order 
 *   the data sources are defined in the RRD, excluding any COMPUTE data 
 *   sources (i.e. if the third data source DST is COMPUTE, the third input 
 *   value will be mapped to the fourth data source in the RRD and so on). 
 *   This is not very error resistant, as you might be sending the wrong 
 *   data into an RRD. 
 * 
 *   The template switch allows you to specify which data sources you are 
 *   going to update and in which order. If the data sources specified in 
 *   the template are not available in the RRD file, the update process 
 *   will abort with an error message.
 *
 *   While it appears possible with the template switch to update data sources 
 *   asynchronously, RRDtool implicitly assigns non-COMPUTE data sources missing 
 *   from the template the *UNKNOWN* value.
 *
 *   Do not specify a value for a COMPUTE DST in the update function. If this 
 *   is done accidentally (and this can only be done using the template switch), 
 *   RRDtool will ignore the value specified for the COMPUTE DST.
 * 
 * N|timestamp:value[:value...]
 *   The data used for updating the RRD was acquired at a certain time. This time 
 *   can either be defined in seconds since 1970-01-01 or by using the letter 'N', 
 *   in which case the update time is set to be the current time. Negative time 
 *   values are subtracted from the current time. An AT_STYLE TIME SPECIFICATION 
 *   MUST NOT be used.
 *
 *   The remaining elements of the argument are DS updates. The order of this 
 *   list is the same as the order the data sources were defined in the RRA. If 
 *   there is no data for a certain data-source, the letter U (e.g., N:0.1:U:1) 
 *   can be specified.
 *
 *   The format of the value acquired from the data source is dependent on the 
 *   data source type chosen. Normally it will be numeric, but the data acquisition 
 *   modules may impose their very own parsing of this parameter as long as the 
 *   colon (:) remains the data source value separator.
 *
 *  NB:  This function uses the thread-safe rrd_update_r() version of the call.
 * 
 */
VALUE rrdtool_update(VALUE self, VALUE otemp, VALUE args)
{
    s_arr   a;        /* varargs in the form of a string array */
    VALUE   rval;     /* our result */
    VALUE   rrd;      /* name of the RRD file to create */
    VALUE   tmpl;     /* DS template */
#ifdef R_RRD_DBG    
    char    buf[NUM_BUF_SZ+1];
#endif
    int result;
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    
    /* initial type checking */
    Check_Type(otemp, T_STRING);
    tmpl = StringValue(otemp);
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "n=[%s] : tmpl=%s", STR2CSTR(rrd), STR2CSTR(tmpl));
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    a = s_arr_new(self, false, false, args);
    
    /* now run the command */
    result = rrd_update_r(STR2CSTR(rrd), STR2CSTR(tmpl), a.len, a.strs);
    /* cleanup */
    s_arr_del(a);

    if (result == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}



/*
 * default calling mechanism for those functions that take
 * the old style argc, argv paramters with rrdtool_name as 
 * the first paramater
 */
VALUE rrdtool_call(VALUE self, RRDtoolFUNC fn, VALUE args)
{
    s_arr     a;        /* varargs in the form of a string array */
    VALUE     rval;     /* our result */
    int result;
    
    reset_rrd_state();
    
    a = s_arr_new(self, true, false, args);
    
    /* now run the command */
    result = fn(a.len, a.strs);

    /* cleanup */
    s_arr_del(a);

    if (result == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}


/*
 * Document-method: tune
 * 
 * call-seq:
 *   rrd.tune(varargs) -> [Qnil|Qtrue]
 * 
 *  The tune option allows you to alter some of the basic configuration 
 *  values stored in the header area of a Round Robin Database (RRD).
 *
 *  One application of the tune function is to relax the validation rules 
 *  on an RRD. This allows to fill a new RRD with data available in larger 
 *  intervals than what you would normally want to permit. Be very careful 
 *  with tune operations for COMPUTE data sources. Setting the min, max, 
 *  and heartbeat for a COMPUTE data source without changing the data source 
 *  type to a non-COMPUTE DST WILL corrupt the data source header in the RRD.
 *
 *  A second application of the tune function is to set or alter parameters 
 *  used by the specialized function RRAs for aberrant behavior detection.
 *
 */
VALUE rrdtool_tune(VALUE self, VALUE args)
{
    return rrdtool_call(self, rrd_tune, args);
}

/*
 * Document-method: resize
 * 
 * call-seq:
 *   rrd.resize(varargs) -> [Qnil|Qtrue]
 *
 * The resize function is used to modify the number of rows in an RRA.
 *  
 * rra-num
 *   the RRA you want to alter.
 * 
 * GROW
 *    used if you want to add extra rows to an RRA. The extra rows will be 
 *    inserted as the rows that are oldest. 
 *
 * SHRINK
 *   used if you want to remove rows from an RRA. The rows that will be 
 *   removed are the oldest rows. 
 *
 * rows
 *   the number of rows you want to add or remove.
 * 
 */
VALUE rrdtool_resize(VALUE self, VALUE args)
{
    return rrdtool_call(self, rrd_resize, args);
}


/*
 * Document-method: restore
 * 
 * call-seq:
 *   rrd.restore(varargs) -> [Qnil|Qtrue]
 *
 * 
 */
VALUE rrdtool_restore(VALUE self, VALUE oxml, VALUE orrd, VALUE args)
{
    s_arr   a;        /* varargs in the form of a string array */
    VALUE   rval;     /* our result */
    VALUE   rrd;      /* name of the RRD file to create */
    VALUE   xml;      /* XML template */
#ifdef R_RRD_DBG    
    char    buf[NUM_BUF_SZ+1];
#endif
    int result;
    
    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    
    /* initial type checking */
    Check_Type(oxml, T_STRING);
    xml = StringValue(oxml);
    Check_Type(orrd, T_STRING);
    rrd = StringValue(orrd);
    
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "restore: xml=%s rrd=%s",
             STR2CSTR(xml), STR2CSTR(rrd));
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    a = s_arr_new(self, false, false, args);
    s_arr_push(STR2CSTR(rrd), &a);
    s_arr_push(STR2CSTR(xml), &a);
    s_arr_push(STR2CSTR(xml), &a);
    
    /* now run the command */
    result = rrd_restore(a.len, a.strs);

    /* cleanup */
    s_arr_del(a);

    if (result == -1) {
        RRD_RAISE;
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}


/*
 * TODO: the fetch and xport functions are very similar. Time to refactor!
 *
 */
VALUE data_output_common(RRDtoolOutputFUNC fn, s_arr a)
{
    unsigned long i, j, k, step, count;
    rrd_value_t  *rrd_data;
    char        **header_strings;
    VALUE         data, header, rval = Qnil;
    time_t        start, end;
#ifdef R_RRD_DBG    
    char buf[NUM_BUF_SZ+1];
#endif
    
    fn(a.len, a.strs, &start, &end, &step, 
       &count, &header_strings, &rrd_data);

    // TODO: a leaks memory if check_error is called here
    RRD_CHECK_ERROR;

    /* process the data .. get the header first */
    header = rb_ary_new();
    for (i = 0; i < count; ++i) {
        rb_ary_push(header, rb_str_new2(header_strings[i]));
#ifdef R_RRD_DBG    
        snprintf(buf, NUM_BUF_SZ, "%s: header: n=[%s]",
                 (fn == rrd_fetch) ? "fetch" : "xport", header_strings[i]);
        buf[NUM_BUF_SZ] = 0;
        _dbug(R_RRD_DEBUG_SIM, buf);
#endif
        free(header_strings[i]);
    }
    free(header_strings);
    
    /* step over the 2d array containing the data */
    k = 0;
    data = rb_ary_new();
    for (i = start+step; i <= end; i += step) {
        VALUE line = rb_ary_new2(count);
        for (j = 0; j < count; j++) {
            rb_ary_store(line, j, rb_float_new(rrd_data[k]));
            k++;
        }
        rb_ary_push(data, line);
    }
    free(rrd_data);
    
    /* now prepare an array for ruby to chew on .. */
    rval = rb_ary_new2(6);
    rb_ary_store(rval, 0, LONG2NUM(start+step));
    rb_ary_store(rval, 1, LONG2NUM(end));

    // used in xport, but not fetch
    rb_ary_store(rval, 2, UINT2NUM(step));
    rb_ary_store(rval, 3, UINT2NUM(count));

    rb_ary_store(rval, 4, header);
    rb_ary_store(rval, 5, data);

    return rval;
}


/*
 * Document-method: fetch
 * 
 * call-seq:
 *   rrd.fetch(str_array) -> (start, end, names, data)
 * 
 * The fetch function is normally used internally by the graph function to 
 * get data from RRDs. fetch will analyze the RRD and try to retrieve the 
 * data in the resolution requested.  The data fetched is printed to stdout. 
 * *UNKNOWN* data is often represented by the string ``NaN'' depending on 
 * your OS's printf function.
 *  
 * Other call options:
 * 
 *   --resolution|-r resolution (default is the highest resolution)
 *       the interval you want the values to have (seconds per value). 
 *       rrdfetch will try to match your request, but it will return data 
 *       even if no absolute match is possible. 
 *
 *  --start|-s start (default end-1day)
 *       start of the time series. A time in seconds since epoch (1970-01-01) 
 *       is required. Negative numbers are relative to the current time. By 
 *       default, one day worth of data will be fetched. 
 *
 *  --end|-e end (default now)
 *       the end of the time series in seconds since epoch. 
 * 
 */
VALUE rrdtool_fetch(VALUE self, VALUE args)
{
    s_arr         a;
    VALUE         rval = Qnil;
    
    reset_rrd_state();
    
    a = s_arr_new(self, true, true, args);

    rval = data_output_common(rrd_fetch, a);
    
    s_arr_del(a);

    RRD_CHECK_ERROR;

    return rval;
}


/*
 * Document-method: xport
 * 
 * call-seq:
 *   RRDtool.xport(str_array) -> (start, end, step, col_cnt, legend, data)
 * 
 * The xport function's main purpose is to write an XML formatted 
 * representation of the data stored in one or several RRDs. It can 
 * also extract numerical reports.
 * 
 * If no XPORT statements are found, there will be no output.
 * 
 *  -s|--start seconds (default end-1day)
 *     The time when the exported range should begin. Time in 
 *     seconds since epoch (1970-01-01) is required. Negative 
 *     numbers are relative to the current time. By default one 
 *     day worth of data will be printed. 
 *  
 *  -e|--end seconds (default now)
 *     The time when the exported range should end. Time in 
 *     seconds since epoch. 
 *  
 *  -m|--maxrows rows (default 400 rows)
 *     This works like the -w|--width parameter of rrdgraph. In 
 *     fact it is exactly the same, but the parameter was renamed 
 *     to describe its purpose in this module. See rrdgraph 
 *     documentation for details. 
 *  
 *  --step value (default automatic)
 *     See the rrdgraph manpage documentation. 
 *  
 *  DEF:vname=rrd:ds-name:CF
 *     See rrdgraph documentation. 
 *  
 *  CDEF:vname=rpn-expression
 *     See rrdgraph documentation. 
 *  
 *  XPORT:vname::legend
 *     At least one XPORT statement should be present. The values 
 *     referenced by vname are printed. Optionally add a legend.
 */
VALUE rrdtool_xport(VALUE self, VALUE args)
{
    s_arr         a;
    unsigned long i, j, k, step, col_cnt;
    rrd_value_t  *rrd_data;
    char        **legend_v;
    VALUE         data, legends, rval = Qnil;
    time_t        start, end;
#ifdef R_RRD_DBG    
    char          buf[NUM_BUF_SZ+1];
#endif
    
    reset_rrd_state();
    
    a = s_arr_new(self, false, true, args);
    
    rrd_xport(a.len, a.strs, 0, &start, &end, &step, 
              &col_cnt, &legend_v, &rrd_data);

    s_arr_del(a);

    RRD_CHECK_ERROR;
    
    /* process the data .. get the legends first */
    legends = rb_ary_new();
    for (i = 0; i < col_cnt; i++) {
        rb_ary_push(legends, rb_str_new2(legend_v[i]));
#ifdef R_RRD_DBG    
        snprintf(buf, NUM_BUF_SZ, "xport: names: n=[%s]", legend_v[i]);
        buf[NUM_BUF_SZ] = 0;
        _dbug(R_RRD_DEBUG_SIM, buf);
#endif
        free(legend_v[i]);
    }
    free(legend_v);
    
    /* step over the 2d array containing the data */
    k = 0;
    data = rb_ary_new();
    for (i = start+step; i <= end; i += step) {
        VALUE line = rb_ary_new2(col_cnt);
        for (j = 0; j < col_cnt; j++) {
            rb_ary_store(line, j, rb_float_new(rrd_data[k]));
            k++;
        }
        rb_ary_push(data, line);
    }
    free(rrd_data);
    
    /* now prepare an array for ruby to chew on .. */
    rval = rb_ary_new2(6);
    rb_ary_store(rval, 0, LONG2NUM(start+step));
    rb_ary_store(rval, 1, LONG2NUM(end));
    rb_ary_store(rval, 2, UINT2NUM(step));
    rb_ary_store(rval, 3, UINT2NUM(col_cnt));
    rb_ary_store(rval, 4, legends);
    rb_ary_store(rval, 5, data);

    return rval;
}



/*
 * Document-method: graph
 * 
 * call-seq:
 *   RRDtool.graph(arg_array) -> [Qnil|Qtrue]
 *
 * The graph function generates an image from the data values in an RRD.
 *
 *
 */
VALUE rrdtool_graph(VALUE self, VALUE args)
{
    s_arr  a;
    char **calcpr, **p;
    VALUE  result, print_results;
    int    xsize, ysize;
    double ymin, ymax;
    
    reset_rrd_state();
    
    a = s_arr_new(self, false, true, args);
    
    rrd_graph(a.len, a.strs, &calcpr, &xsize, &ysize, NULL, &ymin, &ymax);

    s_arr_del(a);

    RRD_CHECK_ERROR;
              
    result = rb_ary_new2(3);
    print_results = rb_ary_new();
    p = calcpr;
    for (p = calcpr; p && *p; p++) {
        rb_ary_push(print_results, rb_str_new2(*p));
        free(*p);
    }
    free(calcpr);
    
    rb_ary_store(result, 0, print_results);
    rb_ary_store(result, 1, INT2NUM(xsize));
    rb_ary_store(result, 2, INT2NUM(ysize));
    return result;
}


/*
 * Document-method: info
 * 
 * call-seq:
 *   rrd.info -> [Qnil|T_HASH]
 *
 *  The info function prints the header information from an RRD in a 
 *  parsing friendly format.
 *
 */
VALUE rrdtool_info(VALUE self)
{
    VALUE   rrd;        /* rrd database filename */
    VALUE   rval;       /* our result */
    info_t *data, *p;   /* this is what rrd_info()returns */

    reset_rrd_state();
    
    rrd = rb_iv_get(self, "@rrdname");
    data = rrd_info_r(STR2CSTR(rrd));

    RRD_CHECK_ERROR;

    rval = rb_hash_new();
    while (data) {
        VALUE key = rb_str_new2(data->key);
        switch (data->type) {
        case RD_I_VAL:
            if (isnan(data->value.u_val)) {
                rb_hash_aset(rval, key, rb_str_new2("Nil"));
            }
            else {
                rb_hash_aset(rval, key, rb_float_new(data->value.u_val));
            }
            break;
        case RD_I_CNT:
            rb_hash_aset(rval, key, UINT2NUM(data->value.u_cnt));
            break;
        case RD_I_STR:
            rb_hash_aset(rval, key, rb_str_new2(data->value.u_str));
            free(data->value.u_str);
            break;
        default:
            rb_hash_aset(rval, key, rb_str_new2("-UNKNOWN-"));
        }
        p = data;
        data = data->next;
	free(p->key);
        free(p);
    }
    return rval;
}


/*
 * return the rrdname instance variable
 */
static VALUE rrdtool_rrdname(VALUE self) {
    VALUE rrdname;
    rrdname = rb_iv_get(self, "@rrdname");
    return rrdname;
}


/*
 * class initialization makes a context
 */
static VALUE rrdtool_initialize(VALUE self, VALUE ofname) {
#ifdef R_RRD_DBG    
    char  buf[NUM_BUF_SZ+1];
#endif
    VALUE rrdname;
    
    rrdname = StringValue(ofname);
    rb_iv_set(self, "@rrdname", rrdname);
#ifdef R_RRD_DBG    
    snprintf(buf, NUM_BUF_SZ, "rrdname=[%s]", STR2CSTR(rrdname));
    buf[NUM_BUF_SZ] = 0;
    _dbug(R_RRD_DEBUG_SIM, buf);
#endif
    return self;
}


/*
 * Define and create the Ruby objects and methods
 */
void Init_RRDtool() 
{
    cRRDtool = rb_define_class("RRDtool", rb_cObject);
    
    rb_eRRDtoolError = rb_define_class("RRDtoolError", rb_eStandardError);
    
    rb_define_method(cRRDtool, "initialize", rrdtool_initialize, 1);
    rb_define_method(cRRDtool, "rrdname",    rrdtool_rrdname, 0);
    rb_define_method(cRRDtool, "create",     rrdtool_create, 3);
    rb_define_method(cRRDtool, "update",     rrdtool_update, 2);
    rb_define_method(cRRDtool, "fetch",      rrdtool_fetch, 1);
    rb_define_method(cRRDtool, "restore",    rrdtool_restore, 3);
    rb_define_method(cRRDtool, "tune",       rrdtool_tune, 1);
    rb_define_method(cRRDtool, "last",       rrdtool_last, 0);
    rb_define_method(cRRDtool, "first",      rrdtool_first, 1);
    rb_define_method(cRRDtool, "resize",     rrdtool_resize, 1);
    rb_define_method(cRRDtool, "info",       rrdtool_info, 0);
#ifdef HAVE_RRD_DUMP_R_2
    rb_define_method(cRRDtool, "dump",       rrdtool_dump, 1);
#endif
    /* version() is really a library function */
    rb_define_singleton_method(cRRDtool, "version", rrdtool_version, 0);
    rb_define_singleton_method(cRRDtool, "graph",   rrdtool_graph, 1);
    rb_define_singleton_method(cRRDtool, "xport",   rrdtool_xport, 1);
}
