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


static RGWMetadataHandler *user_meta_handler = NULL;

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

  int ret = rgw_get_system_obj(store, obj_ctx, bucket, key, bl, NULL, &e.mtime);//赋值uid (username:bwcpn)
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
 * 包括用户限额 等所有信息
 * returns: 0 on success, -ERR# on failure (including nonexistence)
 */
extern int rgw_get_user_info_by_access_key(RGWRados *store, string& access_key, RGWUserInfo& info,
                                           RGWObjVersionTracker *objv_tracker, time_t *pmtime)
{
  int uid;
  rgw_bucket sys_user_bucket;
  int ret = 0;
  
  sys_user_bucket.name = G.sys_user_bucket_root;
  
  uid = getuid(access_key.c_str());
  if (uid < 0)
  {
    return -1;
  }
  info.auid = uid;
  info.display_name = access_key;
  info.user_id = access_key;
  //info.max_buckets = ;
  ret = rgw_get_user_info_from_index(store, access_key, sys_user_bucket/*store->zone.user_keys_pool*/, info, objv_tracker, pmtime);

  info.display_name = access_key;
  info.user_id = access_key;
  return ret;
}
struct RGWUserCompleteInfo {
  RGWUserInfo info;
  map<string, bufferlist> attrs;
  bool has_attrs;

  RGWUserCompleteInfo()
    : has_attrs(false)
  {}

  void dump(Formatter * const f) const {
    info.dump(f);
    encode_json("attrs", attrs, f);
  }

  void decode_json(JSONObj *obj) {
    decode_json_obj(info, obj);
    has_attrs = JSONDecoder::decode_json("attrs", attrs, obj);
  }
};

class RGWUserMetadataObject : public RGWMetadataObject {
  RGWUserCompleteInfo uci;
public:
  RGWUserMetadataObject(const RGWUserCompleteInfo& _uci, obj_version& v, time_t m)
      : uci(_uci) {
    objv = v;
    mtime = m;
  }

  void dump(Formatter *f) const {
    uci.dump(f);
  }
};

class RGWUserMetadataHandler : public RGWMetadataHandler {
public:
  string get_type() { return "user"; }

  int get(RGWRados *store, string& entry, RGWMetadataObject **obj) {
    RGWUserCompleteInfo uci;
    RGWObjVersionTracker objv_tracker;
    time_t mtime;

    int ret = rgw_get_user_info_by_uid(store, entry, uci.info, &objv_tracker,
                                       &mtime, NULL, &uci.attrs);
    if (ret < 0) {
      return ret;
    }

    RGWUserMetadataObject *mdo = new RGWUserMetadataObject(uci, objv_tracker.read_version, mtime);
    *obj = mdo;

    return 0;
  }

  int put(RGWRados *store, string& entry, RGWObjVersionTracker& objv_tracker,
          time_t mtime, JSONObj *obj, sync_type_t sync_mode) {
    RGWUserCompleteInfo uci;

    decode_json_obj(uci, obj);

    map<string, bufferlist> *pattrs = NULL;
    if (uci.has_attrs) {
      pattrs = &uci.attrs;
    }

    RGWUserInfo old_info;
    time_t orig_mtime;
    int ret = rgw_get_user_info_by_uid(store, entry, old_info, &objv_tracker, &orig_mtime);
    if (ret < 0 && ret != -ENOENT)
      return ret;

    // are we actually going to perform this put, or is it too old?
    if (ret != -ENOENT &&
        !check_versions(objv_tracker.read_version, orig_mtime,
			objv_tracker.write_version, mtime, sync_mode)) {
      return STATUS_NO_APPLY;
    }

    ret = 0;//rgw_store_user_info(store, uci.info, &old_info, &objv_tracker, mtime, false, pattrs);
    if (ret < 0) {
      return ret;
    }

    return STATUS_APPLIED;
  }

  struct list_keys_info {
    RGWRados *store;
//    RGWListRawObjsCtx ctx;
  };

  int remove(RGWRados *store, string& entry, RGWObjVersionTracker& objv_tracker) {
    RGWUserInfo info;
    int ret = rgw_get_user_info_by_uid(store, entry, info, &objv_tracker);
    if (ret < 0)
      return ret;

    return rgw_delete_user(store, info, objv_tracker);
  }

  void get_pool_and_oid(RGWRados *store, const string& key, rgw_bucket& bucket, string& oid) {
    oid = key;
    bucket.name = G.sys_user_bucket_root;//store->zone.user_uid_pool;
  }

  int list_keys_init(RGWRados *store, void **phandle)
  {
    list_keys_info *info = new list_keys_info;

    info->store = store;

    *phandle = (void *)info;

    return 0;
  }

  int list_keys_next(void *handle, int max, list<string>& keys, bool *truncated) {
    list_keys_info *info = static_cast<list_keys_info *>(handle);

    string no_filter;

    keys.clear();

    RGWRados *store = info->store;

    list<string> unfiltered_keys;

    int ret = 0;//store->list_raw_objects(rgw_bucket root(G.sys_user_bucket_root.c_str()),/* store->zone.user_uid_pool, */ no_filter,
//                                      max, info->ctx, unfiltered_keys, truncated);
    if (ret < 0)
      return ret;

    // now filter out the buckets entries
    list<string>::iterator iter;
    for (iter = unfiltered_keys.begin(); iter != unfiltered_keys.end(); ++iter) {
      string& k = *iter;

      if (k.find(".buckets") == string::npos) {
        keys.push_back(k);
      }
    }

    return 0;
  }

  void list_keys_complete(void *handle) {
    list_keys_info *info = static_cast<list_keys_info *>(handle);
    delete info;
  }
};

void rgw_user_init(RGWRados *store)
{
  uinfo_cache.init(store);

  user_meta_handler = new RGWUserMetadataHandler;
  store->meta_mgr->register_handler(user_meta_handler);
}

/**
 * delete a user's presence from the RGW system.
 * First remove their bucket ACLs, then delete them
 * from the user and user email pools. This leaves the pools
 * themselves alone, as well as any ACLs embedded in object xattrs.
 */
int rgw_delete_user(RGWRados *store, RGWUserInfo& info, RGWObjVersionTracker& objv_tracker) 
{
  return 0;
}
