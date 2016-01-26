/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_tools.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/25 15:33:57
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include <errno.h>

#include "common/errno.h"

#include "include/types.h"

#include "porting_common.h"
#include "porting_rados.h"
#include "porting_tools.h"
#include "porting_user.h"
#include "common/shell.h"
#include "global/global.h"

#define dout_subsys ceph_subsys_rgw

#define READ_CHUNK_LEN (512 * 1024)

static map<string, string> ext_mime_map;
int sys_info::sync_stat(region_e op, const char *var_name, const char *val)
{
    FILE *fp = NULL;
    char stat_buf[200];
    
    fp = fopen(file.c_str(), "wb+");
    if (!fp)
    {
        return -1;
    }
    if (op == PART)
    {
        
    }
    else
    {
        sprintf(stat_buf, "total_size:%lld\n"
            "num_entries:%lld\n"
            "total_size_rounded:%lld\n",
            total_size, num_entries, total_size_rounded);
        fputs(stat_buf, fp);
        
    }
    fclose(fp);

    return 0;
}
int sys_info:: sync_user(region_e op, const char *var_name, const char *val)
{
    FILE *fp = NULL;
    char user_buf[512]={0};
#if 0    
    fp = fopen(file.c_str(), "wb+");
    if (!fp)
    {
        return -1;
    }
    if (op == PART)
    {
        
    }
    else
    {
        sprintf(stat_buf, "total_size:%lld\n"
            "num_entries:%lld\n"
            "total_size_rounded:%lld\n",
            total_size, num_entries, total_size_rounded);
        fputs(stat_buf, fp);
        
    }
    fclose(fp);
#endif    
    return 0;    
}
void sys_info::set_params(void* obj, category_e category, const char *name, const char *val)
{
    sys_info *pobj = (sys_info *)obj;

    switch(category)
    {
        case USER:
            
            pobj->sync_user(!name ? FULL : PART, name, val);
            break;
        case BUCNET_STAT:
            pobj->sync_stat(!name ? FULL : PART, name, val);
            break;
        default:
            break;
    }
}
void sys_info::get_params(void* obj, const char *name, const char *val)
{
    sys_info *pobj = (sys_info *)obj;
    if (string(name) == "display_name")
    {
        pobj->display_name = val;
    }
    if (string(name) == "user_email")
    {
        pobj->user_email = val;
    }
    if (string(name) == "max_buckets")
    {
        pobj->max_buckets = atoi(val);
    }
    if (string(name) == "bucket_max_size_kb")
    {
        pobj->bucket_max_size_kb = atoi(val);
    }
    if (string(name) == "bucket_max_objects")
    {
        pobj->bucket_max_objects = atoi(val);
    }
    if (string(name) == "bucket_enabled")
    {
        pobj->bucket_enabled = atoi(val) == 1 ? true :false;
    }
    if (string(name) == "user_max_size_kb")
    {
        pobj->user_max_size_kb =  atoi(val);
    }    
    if (string(name) == "user_max_objects")
    {
        pobj->user_max_objects = atoi(val);
    }  
    if (string(name) == "user_enabled")
    {
        pobj->user_enabled = atoi(val) == 1 ? true :false;
    }
    if (string(name) == "total_size")
    {
        pobj->total_size = atoi(val);
    }   
    if (string(name) == "total_size_rounded")
    {
        pobj->total_size_rounded = atoi(val);
    }   
    if (string(name) == "num_entries")
    {
        pobj->num_entries = atoi(val);
    }       
}

int rgw_put_system_obj(RGWRados *rgwstore, rgw_bucket& bucket, string& oid, const char *data, size_t size, bool exclusive,
                       RGWObjVersionTracker *objv_tracker, time_t set_mtime, map<string, bufferlist> *pattrs)
{
#if 1
  //设置属性 若失败则创建存储池(ceph)/创建桶/对象?
  map<string,bufferlist> no_attrs;
  if (!pattrs)
    pattrs = &no_attrs;

  rgw_obj obj(bucket, oid);

  int ret = rgwstore->put_system_obj(NULL, obj, data, size, exclusive, NULL, *pattrs, objv_tracker, set_mtime);

  if (ret == -ENOENT) {
    ret = rgwstore->create_pool(bucket);
    if (ret >= 0)
      ret = rgwstore->put_system_obj(NULL, obj, data, size, exclusive, NULL, *pattrs, objv_tracker, set_mtime);
  }

  return ret;
#endif
  return 0;
}
int rgw_get_system_obj(RGWRados *rgwstore, RGWObjectCtx& obj_ctx, rgw_bucket& bucket, const string& key, bufferlist& bl,
                       RGWObjVersionTracker *objv_tracker, time_t *pmtime, map<string, bufferlist> *pattrs,
                       rgw_cache_entry_info *cache_info)
{
  struct rgw_err err;
  bufferlist::iterator iter;
  int request_len = READ_CHUNK_LEN;
  rgw_obj obj(bucket, key);
  /* :TODO:2016/1/21 16:35:13:hwt:  获取用户头信息*/
  RGWUID uid;
  RGWUserInfo user_inf;
    
//push uid
  uid.user_id = key;
  ::encode(uid, bl);
  //bl.append(bl_tmp);
//push user info
  string conf_path = G.sys_user_bucket_root + string("/") + string(".") + uid.user_id + string(".conf");
  sys_info info;
  
  parse_conf(conf_path.c_str(), &info, ":",(FUNC)(&info.get_params));

  user_inf.max_buckets = info.max_buckets;
  user_inf.bucket_quota.enabled = info.bucket_enabled;
    
  user_inf.bucket_quota.max_size_kb = info.bucket_max_size_kb;
  user_inf.bucket_quota.max_objects = info.bucket_max_objects;

  user_inf.user_quota.enabled = info.user_enabled;
  user_inf.user_quota.max_size_kb =  info.user_max_size_kb;
  user_inf.user_quota.max_objects = info.user_max_objects;
  user_inf.auid = getuid(key.c_str());

  ::encode(user_inf, bl);
  
  if (cache_info)
  {
      cache_info->cache_locator = key;
  }
  /* :TODO:End---  */
#if 0
  do {
    RGWRados::SystemObject source(rgwstore, obj_ctx, obj);
    RGWRados::SystemObject::Read rop(&source);

    rop.stat_params.attrs = pattrs;
    rop.stat_params.lastmod = pmtime;
    rop.stat_params.perr = &err;

    int ret = rop.stat(objv_tracker);
    if (ret < 0)
      return ret;

    rop.read_params.cache_info = cache_info;

    ret = rop.read(0, request_len - 1, bl, objv_tracker);
    if (ret == -ECANCELED) {
      /* raced, restart */
      continue;
    }
    if (ret < 0)
      return ret;

    if (ret < request_len)
      break;
    bl.clear();
    request_len *= 2;
  } while (true);
#endif
  return 0;
}
void parse_mime_map_line(const char *start, const char *end)
{
  char line[end - start + 1];
  strncpy(line, start, end - start);
  line[end - start] = '\0';
  char *l = line;
#define DELIMS " \t\n\r"

  while (isspace(*l))
    l++;

  char *mime = strsep(&l, DELIMS);
  if (!mime)
    return;

  char *ext;
  do {
    ext = strsep(&l, DELIMS);
    if (ext && *ext) {
      ext_mime_map[ext] = mime;
    }
  } while (ext);
}


void parse_mime_map(const char *buf)
{
  const char *start = buf, *end = buf;
  while (*end) {
    while (*end && *end != '\n') {
      end++;
    }
    parse_mime_map_line(start, end);
    end++;
    start = end;
  }
}

static int ext_mime_map_init(CephContext *cct, const char *ext_map)
{
  int fd = open(ext_map, O_RDONLY);
  char *buf = NULL;
  int ret;
  if (fd < 0) {
    ret = -errno;
    ldout(cct, 0) << "ext_mime_map_init(): failed to open file=" << ext_map << " ret=" << ret << dendl;
    return ret;
  }

  struct stat st;
  ret = fstat(fd, &st);
  if (ret < 0) {
    ret = -errno;
    ldout(cct, 0) << "ext_mime_map_init(): failed to stat file=" << ext_map << " ret=" << ret << dendl;
    goto done;
  }

  buf = (char *)malloc(st.st_size + 1);
  if (!buf) {
    ret = -ENOMEM;
    ldout(cct, 0) << "ext_mime_map_init(): failed to allocate buf" << dendl;
    goto done;
  }

  ret = read(fd, buf, st.st_size + 1);
  if (ret != st.st_size) {
    // huh? file size has changed, what are the odds?
    ldout(cct, 0) << "ext_mime_map_init(): raced! will retry.." << dendl;
    free(buf);
    close(fd);
    return ext_mime_map_init(cct, ext_map);
  }
  buf[st.st_size] = '\0';

  parse_mime_map(buf);
  ret = 0;
done:
  free(buf);
  close(fd);
  return ret;
}

const char *rgw_find_mime_by_ext(string& ext)
{
  map<string, string>::iterator iter = ext_mime_map.find(ext);
  if (iter == ext_mime_map.end())
    return NULL;

  return iter->second.c_str();
}

int rgw_tools_init(CephContext *cct)
{
  int ret = ext_mime_map_init(cct, "mine"/*cct->_conf->rgw_mime_types_file.c_str()*/);
  if (ret < 0)
    return ret;

  return 0;
}

void rgw_tools_cleanup()
{
  ext_mime_map.clear();
}


