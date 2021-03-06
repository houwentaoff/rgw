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
#include <string>

using namespace std;

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
    server_uid           = 0;
    user_name            = "";
    group_name           = "";
    rgw_s3_auth_use_rados   = false;
    rgw_expose_bucket       = true;
    rgw_cache_lru_size   = 0;
    rgw_num_rados_handles  = 0;
    rgw_cache_enabled       = false; 
    rgw_bucket_default_quota_max_objects = -1;
    rgw_bucket_default_quota_max_size    = -1;
    rgw_user_default_quota_max_objects   = -1;
    rgw_user_default_quota_max_size      = -1;
    sys_user_bucket_root  = "";
    rgw_multipart_min_part_size = 0;
    rgw_multipart_part_upload_limit = 0;
    
    
}
void globals::set_global_params(void *obj, const char *name, const char *val)
{
    globals *pobj = (globals *)obj;
    if (string(name) == "buckets_root")
    {
        pobj->buckets_root = val;
    }
    if (string(name) =="rgw_max_chunk_size")
    {
        pobj->rgw_max_chunk_size = atoi(val);
    }
    if (string(name) =="rgw_max_put_size")
    {
        pobj->rgw_max_put_size = atoi(val);
    }
    if (string(name) =="rgw_thread_pool_size")
    {
        pobj->rgw_thread_pool_size = atoi(val);
    }
    if (string(name) =="rgw_list_buckets_max_chunk")
    {
        pobj->rgw_list_buckets_max_chunk = atoi(val);
    }
    if (string(name) =="logFileSize")
    {
        pobj->logFileSize = atoi(val);
    }
    if (string(name) =="server_uid")
    {
        pobj->server_uid = atoi(val);
    }   
    if (string(name) =="user_name")
    {
        pobj->user_name = val;
    }   
    if (string(name) =="group_name")
    {
        pobj->group_name = val;
    }       
    if (string(name) =="rgw_s3_auth_use_rados")
    {
        pobj->rgw_s3_auth_use_rados = atoi(val) == 1 ? true :false;
    }           
    if (string(name) =="rgw_expose_bucket")
    {
        pobj->rgw_expose_bucket = atoi(val) == 1 ? true :false;
    }
    if (string(name) =="rgw_cache_lru_size")
    {
        pobj->rgw_cache_lru_size = atoi(val);
    }    
    if (string(name) =="rgw_num_rados_handles")
    {
        pobj->rgw_num_rados_handles = atoi(val);
    }
    if (string(name) =="rgw_cache_enabled")
    {
        pobj->rgw_cache_enabled = atoi(val) == 1 ? true :false;
    }
    if (string(name) =="rgw_bucket_default_quota_max_objects")
    {
        pobj->rgw_bucket_default_quota_max_objects = atoi(val);
    }    
    if (string(name) =="rgw_bucket_default_quota_max_size")
    {
        pobj->rgw_bucket_default_quota_max_size = atoi(val);
    }
    if (string(name) =="rgw_user_default_quota_max_objects")
    {
        pobj->rgw_user_default_quota_max_objects = atoi(val);
    }
    if (string(name) =="rgw_user_default_quota_max_size")
    {
        pobj->rgw_user_default_quota_max_size = atoi(val);
    }  
    if (string(name) =="sys_user_bucket_root")
    {
        pobj->sys_user_bucket_root = val;
    }
    if (string(name) =="rgw_multipart_min_part_size")
    {
        pobj->rgw_multipart_min_part_size = atoi(val);
    }
    if (string(name) =="rgw_multipart_part_upload_limit")
    {
        pobj->rgw_multipart_part_upload_limit = atoi(val);
    }}

