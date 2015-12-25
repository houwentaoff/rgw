/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  global.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/22 10:36:26
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include "global.h"

struct globals globals;
struct globals *ptr_to_globals;

void globals::init()
{
    memset(&logFile, 0, sizeof(logFile_t));
    logFileSize          = 0;
    logFileRotate        = 0;
    memset(&lastLogTime, 0, sizeof(time_t));
    exe                  = NULL;
    root                 = NULL;
    hostname             = "";
    rgw_thread_pool_size = 0;
    rgw_list_buckets_max_chunk = 0;
    rgw_relaxed_s3_bucket_names        = true;
}
