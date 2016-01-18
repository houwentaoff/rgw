// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <errno.h>

#include <string>
#include <map>

#include "common/errno.h"
#include "common/Formatter.h"
#include "common/ceph_json.h"
#include "common/RWLock.h"
//#include "rgw_rados.h"
#include "porting_acl.h"

#include "include/types.h"
#include "porting_user.h"
#include "rgw_string.h"

// until everything is moved from rgw_common
#include "porting_common.h"

#include "porting_bucket.h"
#include "global/global.h"

#define dout_subsys ceph_subsys_rgw

using namespace std;


//static RGWMetadataHandler *user_meta_handler = NULL;

struct user_info_entry {
  RGWUserInfo info;
  RGWObjVersionTracker objv_tracker;
  time_t mtime;
};

static RGWChainedCacheImpl<user_info_entry> uinfo_cache;
int rgw_get_user_info_from_index(RGWRados *store, string& key, rgw_bucket& bucket, RGWUserInfo& info,
                                 RGWObjVersionTracker *objv_tracker, time_t *pmtime)
{
  user_info_entry e;
  if (uinfo_cache.find(key, &e)) {
    info = e.info;
    if (objv_tracker)
      *objv_tracker = e.objv_tracker;
    if (pmtime)
      *pmtime = e.mtime;
    return 0;
  }

  bufferlist bl;
  RGWUID uid;
  RGWObjectCtx obj_ctx(store);

  int ret = rgw_get_system_obj(store, obj_ctx, bucket, key, bl, NULL, &e.mtime);
  if (ret < 0)
    return ret;

  rgw_cache_entry_info cache_info;

  bufferlist::iterator iter = bl.begin();
  try {
    ::decode(uid, iter);
    int ret = rgw_get_user_info_by_uid(store, uid.user_id, e.info, &e.objv_tracker, NULL, &cache_info);
    if (ret < 0) {
      return ret;
    }
  } catch (buffer::error& err) {
    ldout(store->ctx(), 0) << "ERROR: failed to decode user info, caught buffer::error" << dendl;
    return -EIO;
  }

  list<rgw_cache_entry_info *> cache_info_entries;
  cache_info_entries.push_back(&cache_info);

  uinfo_cache.put(store, key, &e, cache_info_entries);

  info = e.info;
  if (objv_tracker)
    *objv_tracker = e.objv_tracker;
  if (pmtime)
    *pmtime = e.mtime;

  return 0;
}

/**
 * Given a uid, finds the user info associated with it.
 * returns: 0 on success, -ERR# on failure (including nonexistence)
 */
int rgw_get_user_info_by_uid(RGWRados *store,
                             string& uid,
                             RGWUserInfo& info,
                             RGWObjVersionTracker *objv_tracker,
                             time_t *pmtime,
                             rgw_cache_entry_info *cache_info,
                             map<string, bufferlist> *pattrs)
{
  bufferlist bl;
  RGWUID user_id;
 /* :TODO:01/18/2016 05:26:05 PM:hwt:  */
  rgw_bucket  bucket;
 /* :TODO:End---  */
  RGWObjectCtx obj_ctx(store);
  int ret = rgw_get_system_obj(store, obj_ctx, bucket/*store->zone.user_uid_pool*/, uid, bl, objv_tracker, pmtime, pattrs, cache_info);
  if (ret < 0) {
    return ret;
  }

  bufferlist::iterator iter = bl.begin();
  try {
    ::decode(user_id, iter);
    if (user_id.user_id.compare(uid) != 0) {
      lderr(store->ctx())  << "ERROR: rgw_get_user_info_by_uid(): user id mismatch: " << user_id.user_id << " != " << uid << dendl;
      return -EIO;
    }
    if (!iter.end()) {
      ::decode(info, iter);
    }
  } catch (buffer::error& err) {
    ldout(store->ctx(), 0) << "ERROR: failed to decode user info, caught buffer::error" << dendl;
    return -EIO;
  }

  return 0;
}

/**
 * Get the anonymous (ie, unauthenticated) user info.
 */
void rgw_get_anon_user(RGWUserInfo& info)
{
  info.user_id = RGW_USER_ANON_ID;
  info.display_name.clear();
  info.access_keys.clear();
}

bool rgw_user_is_authenticated(RGWUserInfo& info)
{
  return (info.user_id != RGW_USER_ANON_ID);
}

/**
 * Given an access key, finds the user info associated with it.
 * returns: 0 on success, -ERR# on failure (including nonexistence)
 */
extern int rgw_get_user_info_by_access_key(RGWRados *store, string& access_key, RGWUserInfo& info,
                                           RGWObjVersionTracker *objv_tracker, time_t *pmtime)
{
  int uid;

  uid = getuid(access_key.c_str());
  if (uid < 0)
  {
    return -1;
  }
  info.auid = uid;
  info.display_name = access_key;
  info.user_id = access_key;
  //info.max_buckets = ;
  return 0;//rgw_get_user_info_from_index(store, access_key, store->zone.user_keys_pool, info, objv_tracker, pmtime);
}

