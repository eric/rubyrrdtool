/* -----
 * file:   rubyrrdtool.c
 * date:   $Date: 2005/08/04 01:16:08 $
 * init:   2005-07-26
 * vers:   $Version$
 * auth:   $Author: probertm $
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
#include <ruby.h>
#include <rrd.h>

#define  R_RRD_DBG 1     /* printf debugging */

void _dbug(char *s) {
#ifdef R_RRD_DBG
    printf(" --> %s\n", s);
#endif
}

VALUE mRRDtool;
VALUE rb_eRRDtoolError;

/*
 * Define a structure to store counted array of strings
 */
typedef struct string_arr_t {
    int len;
    char **strings;
} string_arr;


#define NUM_BUF_SZ 100

/*
 * create a new array of strings based on a Ruby array
 */
string_arr string_arr_new(VALUE rb_strings)
{
    string_arr a;
    char buf[NUM_BUF_SZ];
    int i;
   
    Check_Type(rb_strings, T_ARRAY);
    a.len = RARRAY(rb_strings)->len;
    a.strings = ALLOC_N(char*, a.len);

    for (i = 0; i < a.len; i++) {
        VALUE v = rb_ary_entry(rb_strings, i);
        switch (TYPE(v)) {
        case T_STRING:
            a.strings[i] = strdup(STR2CSTR(v));
            _dbug(a.strings[i]);
            break;
            
        case T_BIGNUM:
        case T_FIXNUM:
            snprintf(buf, NUM_BUF_SZ-1, "%d", FIX2INT(v));
            a.strings[i] = strdup(buf);
            _dbug(a.strings[i]);
            break;
            
        default:
            rb_raise(rb_eTypeError, "invalid argument for string array");
            break;
        }
    }
    sprintf(buf, "len=%d", a.len);
    _dbug(buf);
    
    return a;
}

/*
 * free up the memory we have allocated
 */
void string_arr_delete(string_arr a)
{
    int i;

    /* skip dummy first entry */
    for (i = 1; i < a.len; i++) {
        free(a.strings[i]);
    }

    free(a.strings);
}

/*
 * reset teh internal state
 */
void reset_rrd_state()
{
    optind = 0; 
    opterr = 0;
    rrd_clear_error();
}


/*
 * define a macro for easy error checking
 */
typedef int (*RRDFUNC)(int argc, char ** argv);
#define RRD_CHECK_ERROR  \
    if (rrd_test_error()) \
        rb_raise(rb_eRRDError, rrd_get_error()); \
    rrd_clear_error();

/*
 * Document-method: call
 */
VALUE rrd_call(RRDFUNC func, VALUE args)
{
    string_arr a;

    a = string_arr_new(args);
    reset_rrd_state();
    func(a.len, a.strings);
    string_arr_delete(a);

    RRD_CHECK_ERROR

    return Qnil;
}


/*
 * Document-method: create
 *
 *   call-seq:
 *     RRD.create(filename, pdp_step, last_up, arg_array) -> [Qnil|Qtrue]
 * 
 * The create function of RRDtool lets you set up new Round Robin 
 * Database (RRD) files. The file is created at its final, full size 
 * and filled with *UNKNOWN* data.
 *
 * create filename [--start|-b start time] [--step|-s step] 
 *        [DS:ds-name:DST:heartbeat:min:max] [RRA:CF:xff:steps:rows
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
VALUE rb_rrd_create(VALUE self, VALUE ofname, VALUE ostep, VALUE update, VALUE args)
{
    string_arr     a;        /* varargs in the form of a string array */
    VALUE          rval;     /* our result */
    VALUE          rrd;      /* name of the RRD file to create */
    unsigned long  pdp_step; /* the stepping time interval */
    time_t         last_up;  /* last update time */
#ifdef R_RRD_DBG    
    char           buf[254]; 
#endif
    
    /* initial type checking */
    Check_Type(ofname, T_STRING);
    Check_Type(ostep, T_FIXNUM);
    Check_Type(update, T_BIGNUM);
    
    /* conversion ... */
    rrd = StringValue(ofname);
    pdp_step = NUM2ULONG(ostep);
    last_up  = (time_t)NUM2ULONG(update);
#ifdef R_RRD_DBG    
    sprintf(buf, "n=[%s] : step=%d : up=%d", STR2CSTR(rrd), pdp_step, last_up);
    _dbug(buf);
#endif
    a = string_arr_new(args);
    
    
    /* now run the command */
    if (rrd_create_r(STR2CSTR(rrd), pdp_step, last_up, a.len, a.strings) == -1) {
        rb_raise(rb_eRRDError, rrd_get_error()); \
        rrd_clear_error();
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    string_arr_delete(a);
    return rval;
}


/*
 * Document-method: dump
 *
 *   call-seq:
 *     RRD.dump(filename) -> [Qnil|Qtrue]
 * 
 * The dump function prints the contents of an RRD in human readable (?) 
 * XML format. This format can be read by rrdrestore. Together they allow 
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
 */
VALUE rb_rrd_dump(VALUE self, VALUE o_rrdname)
{
    VALUE   rval;       /* our result */
    VALUE   fname;      /* holds the filename */
    VALUE   rrdname;    /* rrd database filename */
    
    rrdname = StringValue(o_rrdname);
     
    if (rrd_dump_r(STR2CSTR(rrdname)) == -1) {
        rb_raise(rb_eRRDError, rrd_get_error()); \
        rrd_clear_error();
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    return rval;
}



/*
 * Document-method: update
 * 
 * call-seq:
 *   RRD.update(filename, template, arg_array) -> [Qnil|Qtrue]
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
VALUE rb_rrd_update(VALUE self, VALUE ofname, VALUE otemp, VALUE args)
{
    string_arr     a;        /* varargs in the form of a string array */
    VALUE          rval;     /* our result */
    VALUE          rrd;      /* name of the RRD file to create */
    VALUE          tmpl;     /* DS template */
#ifdef R_RRD_DBG    
    char           buf[254]; 
#endif
    
    /* initial type checking */
    Check_Type(ofname, T_STRING);
    Check_Type(otemp, T_STRING);
    
    /* conversion ... */
    rrd  = StringValue(ofname);
    tmpl = StringValue(otemp);
    
#ifdef R_RRD_DBG    
    sprintf(buf, "n=[%s] : temp=%s", STR2CSTR(rrd), STR2CSTR(tmpl));
    _dbug(buf);
#endif
    a = string_arr_new(args);
    
    /* now run the command */
    if (rrd_update_r(STR2CSTR(rrd), STR2CSTR(tmpl), a.len, a.strings) == -1) {
        rb_raise(rb_eRRDError, rrd_get_error()); \
        rrd_clear_error();
        rval = Qnil;
    } else {
        rval = Qtrue;
    }
    string_arr_delete(a);
    return rval;
}


VALUE rb_rrd_resize(VALUE self, VALUE args)
{
    return rrd_call(rrd_resize, args);
}

VALUE rb_rrd_restore(VALUE self, VALUE args)
{
    return rrd_call(rrd_restore, args);
}

VALUE rb_rrd_tune(VALUE self, VALUE args)
{
    return rrd_call(rrd_tune, args);
}

/*
 * Document-method: fetch
 * 
 * call-seq:
 *   RRD.filename =  name of the RRD you want to fetch the data from. 
 *   RRD.CF       =  consolidation function applied to the data (AVERAGE,MIN,MAX,LAST) 
 *   RRD.array    -> (start, end, names, data)
 * 
 * The fetch function is normally used internally by the graph function to 
 * get data from RRDs. fetch will analyze the RRD and try to retrieve the 
 * data in the resolution requested.  The data fetched is printed to stdout. 
 * *UNKNOWN* data is often represented by the string ``NaN'' depending on 
 * your OS's printf function.
 *  
 * Other call options:
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
 */
VALUE rb_rrd_fetch(VALUE self, VALUE args)
{
    string_arr a;
    unsigned long i, j, k, step, ds_cnt;
    rrd_value_t *raw_data;
    char **raw_names;
    VALUE data, names, result;
    time_t start, end;

    a = string_arr_new(args);
    reset_rrd_state();
    rrd_fetch(a.len, a.strings, &start, &end, &step, &ds_cnt, &raw_names, &raw_data);
    string_arr_delete(a);

    RRD_CHECK_ERROR

    names = rb_ary_new();
    for (i = 0; i < ds_cnt; i++) {
        rb_ary_push(names, rb_str_new2(raw_names[i]));
        free(raw_names[i]);
    }
    free(raw_names);

    k = 0;
    data = rb_ary_new();
    for (i = start; i <= end; i += step) {
        VALUE line = rb_ary_new2(ds_cnt);
        for (j = 0; j < ds_cnt; j++) {
            rb_ary_store(line, j, rb_float_new(raw_data[k]));
            k++;
        }
        rb_ary_push(data, line);
    }
    free(raw_data);
   
    result = rb_ary_new2(4);
    rb_ary_store(result, 0, INT2FIX(start));
    rb_ary_store(result, 1, INT2FIX(end));
    rb_ary_store(result, 2, names);
    rb_ary_store(result, 3, data);
    return result;
}




/*
 * Document-method: graph
 * 
 * call-seq:
 *   RRD.update(filename, template, arg_array) -> [Qnil|Qtrue]
 *
 * The update function feeds new data values into an RRD. The data is time aligned 
 *  (interpolated) according to the properties of the RRD to which the data is written.
 *
 *
 */
VALUE rb_rrd_graph(VALUE self, VALUE args)
{
    string_arr a;
    char **calcpr, **p;
    VALUE result, print_results;
    int i, xsize, ysize;
    double ymin, ymax;

    a = string_arr_new(args);
    reset_rrd_state();
    rrd_graph(a.len, a.strings, &calcpr, &xsize, &ysize, NULL, &ymin, &ymax);
    string_arr_delete(a);

    RRD_CHECK_ERROR

    result = rb_ary_new2(3);
    print_results = rb_ary_new();
    p = calcpr;
    for (p = calcpr; p && *p; p++) {
        rb_ary_push(print_results, rb_str_new2(*p));
        free(*p);
    }
    free(calcpr);
    rb_ary_store(result, 0, print_results);
    rb_ary_store(result, 1, INT2FIX(xsize));
    rb_ary_store(result, 2, INT2FIX(ysize));
    return result;
}

/*
VALUE rb_rrd_info(VALUE self, VALUE args)
{
    string_arr a;
    info_t *p;
    VALUE result;

    a = string_arr_new(args);
    data = rrd_info(a.len, a.strings);
    string_arr_delete(a);

    RRD_CHECK_ERROR

    result = rb_hash_new();
    while (data) {
        VALUE key = rb_str_new2(data->key);
        switch (data->type) {
        case RD_I_VAL:
            if (isnan(data->u_val)) {
                rb_hash_aset(result, key, Qnil);
            }
            else {
                rb_hash_aset(result, key, rb_float_new(data->u_val));
            }
            break;
        case RD_I_CNT:
            rb_hash_aset(result, key, INT2FIX(data->u_cnt));
            break;
        case RD_I_STR:
            rb_hash_aset(result, key, rb_str_new2(data->u_str));
            free(data->u_str);
            break;
        }
        p = data;
        data = data->next;
        free(p);
    }
    return result;
}
*/

VALUE rb_rrd_last(VALUE self, VALUE args)
{
    string_arr a;
    time_t last;

    a = string_arr_new(args);
    reset_rrd_state();
    last = rrd_last(a.len, a.strings);
    string_arr_delete(a);

    RRD_CHECK_ERROR

    return rb_funcall(rb_cTime, rb_intern("at"), 1, INT2FIX(last));
}

void Init_RRD() 
{
    mRRDtool = rb_define_module("RRDtool");
    rb_eRRDError = rb_define_class("RRDtoolError", rb_eStandardError);

    rb_define_module_function(mRRDtool, "create", rb_rrd_create, 4);
    rb_define_module_function(mRRDtool, "dump", rb_rrd_dump, 1);
    rb_define_module_function(mRRDtool, "fetch", rb_rrd_fetch, -2);
    rb_define_module_function(mRRDtool, "graph", rb_rrd_graph, -2);
    rb_define_module_function(mRRDtool, "last", rb_rrd_last, -2);
    rb_define_module_function(mRRDtool, "resize", rb_rrd_resize, -2);
    rb_define_module_function(mRRDtool, "restore", rb_rrd_restore, -2);
    rb_define_module_function(mRRDtool, "tune", rb_rrd_tune, -2);
    rb_define_module_function(mRRDtool, "update", rb_rrd_update, 3);
}
