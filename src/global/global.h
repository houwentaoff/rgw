/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  global.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/22 10:34:44
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <string.h>
#include <sys/types.h>

#include <string>

#define _M(x)                   (x*(1024*1024))/*  x M bytes*/
#define _G(x)                   (x*_M(1024))/*  x G bytes*/

#define G (*ptr_to_globals)

#define INIT_G()  do { \
    ptr_to_globals = (struct globals *)&globals; \
    ptr_to_globals->init(); \
}while (0)

struct globals;
extern struct globals *ptr_to_globals;
extern struct globals globals;

typedef struct logFile_t{
    const char *path;
    int fd;
    unsigned  size;
    unsigned int isRegular;
}logFile_t;

struct globals {
    void init();
    static void set_global_params(void* obj, const char *name, const char *val);    
    /* log */
    logFile_t logFile;
    /* max size of file before rotation */ 
    unsigned logFileSize;
    /* number of rotated message files */  
    unsigned logFileRotate;    
    time_t lastLogTime;
    /* exe path */
    const char *exe;
    /*  cwd path */
    const char *cwd;
    /* root dir /sobey/fics/ */
    const char *root;

    /* hostname */
    std::string hostname;
    /* listen thread zise */
    int rgw_thread_pool_size;
    /*  max buckets to retrieve in a single op when listing user buckets */
    int rgw_list_buckets_max_chunk;
    /* allow bucket name start with '_' */
    bool rgw_relaxed_s3_bucket_names;
    /* bucket root path */
    std::string buckets_root;
    /* max chunk size*/
    uint64_t rgw_max_chunk_size;
    /* nax s3cmd put size */
    long rgw_max_put_size;
    /* server uid  default is 502*/
    uid_t    server_uid;
    /* default pw name*/
    std::string user_name;
    std::string group_name;
    bool rgw_s3_auth_use_rados;
};

#endif
