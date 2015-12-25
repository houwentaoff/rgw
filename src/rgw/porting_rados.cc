/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rados.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 15:14:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "porting_rados.h"
#include "cls/user/cls_user_client.h"
#include "include/rados/librados.hh"
#include "porting_tools.h"
#include "common/dout.h"

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <map>

struct bucket_info_entry {
  RGWBucketInfo info;
  time_t mtime;
  map<string, bufferlist> attrs;
};

int RGWRados::cls_user_list_buckets(rgw_obj& obj,
                                    const string& in_marker, int max_entries,
                                    list<cls_user_bucket_entry>& entries,
                                    string *out_marker, bool *truncated)
{
  librados::ObjectReadOperation op;
  int rc;

  cls_user_bucket_list(op, in_marker, max_entries, entries, out_marker, truncated, &rc);
  if (rc < 0)
    return rc;    

    return 0;
}

int RGWRados::get_bucket_entrypoint_info(RGWObjectCtx& obj_ctx, const string& bucket_name,
                                         RGWBucketEntryPoint& entry_point,
                                         RGWObjVersionTracker *objv_tracker,
                                         time_t *pmtime,
                                         map<string, bufferlist> *pattrs,
                                         rgw_cache_entry_info *cache_info)
{
  bufferlist bl;
  rgw_bucket domain_root;

  int ret = rgw_get_system_obj(this, obj_ctx, domain_root/*zone.domain_root*/, bucket_name, bl, objv_tracker, pmtime, pattrs, cache_info);
  if (ret < 0) {
    return ret;
  }

  bufferlist::iterator iter = bl.begin();
  try {
    ::decode(entry_point, iter);
  } catch (buffer::error& err) {
    ldout(cct, 0) << "ERROR: could not decode buffer info, caught buffer::error" << dendl;
    return -EIO;
  }
  return 0;
}

int RGWRados::get_bucket_info(RGWObjectCtx& obj_ctx, const string& bucket_name, RGWBucketInfo& info,
                              time_t *pmtime, map<string, bufferlist> *pattrs)
{
  bucket_info_entry e;
//  if (binfo_cache.find(bucket_name, &e)) {
//    info = e.info;
//    if (pattrs)
//      *pattrs = e.attrs;
//    if (pmtime)
//      *pmtime = e.mtime;
//    return 0;
//  }

  bufferlist bl;

  RGWBucketEntryPoint entry_point;
  time_t ep_mtime;
  RGWObjVersionTracker ot;
  rgw_cache_entry_info entry_cache_info;
  int ret = get_bucket_entrypoint_info(obj_ctx, bucket_name, entry_point, &ot, &ep_mtime, pattrs, &entry_cache_info);
  if (ret < 0) {
    info.bucket.name = bucket_name; /* only init this field */
    return ret;
  }

  if (entry_point.has_bucket_info) {
    info = entry_point.old_bucket_info;
    info.bucket.oid = bucket_name;
    info.ep_objv = ot.read_version;
//    ldout(cct, 20) << "rgw_get_bucket_info: old bucket info, bucket=" << info.bucket << " owner " << info.owner << dendl;
    return 0;
  }

  /* data is in the bucket instance object, we need to get attributes from there, clear everything
   * that we got
   */
  if (pattrs) {
    pattrs->clear();
  }

//  ldout(cct, 20) << "rgw_get_bucket_info: bucket instance: " << entry_point.bucket << dendl;

  if (pattrs)
    pattrs->clear();

  /* read bucket instance info */

  string oid;
//  get_bucket_meta_oid(entry_point.bucket, oid);

  rgw_cache_entry_info cache_info;

//  ret = get_bucket_instance_from_oid(obj_ctx, oid, e.info, &e.mtime, &e.attrs, &cache_info);
  e.info.ep_objv = ot.read_version;
  info = e.info;
  if (ret < 0) {
    info.bucket.name = bucket_name;
    return ret;
  }

  if (pmtime)
    *pmtime = e.mtime;
  if (pattrs)
    *pattrs = e.attrs;

  list<rgw_cache_entry_info *> cache_info_entries;
  cache_info_entries.push_back(&entry_cache_info);
  cache_info_entries.push_back(&cache_info);


  /* chain to both bucket entry point and bucket instance */
//  if (!binfo_cache.put(this, bucket_name, &e, cache_info_entries)) {
//    ldout(cct, 20) << "couldn't put binfo cache entry, might have raced with data changes" << dendl;
//  }

  return 0;
}

RGWObjState *RGWObjectCtx::get_state(rgw_obj& obj) {
  if (!obj.get_object().empty()) {
    return &objs_state[obj];
  } else {
//    rgw_obj new_obj("zone.domain_root"/*store->zone.domain_root*/, obj.bucket.name);
//    return &objs_state[new_obj];
  }
}
void RGWObjectCtx::invalidate(rgw_obj& obj)
{
  objs_state.erase(obj);
}

void RGWObjectCtx::set_atomic(rgw_obj& obj) {
  if (!obj.get_object().empty()) {
    objs_state[obj].is_atomic = true;
  } else {
//    rgw_obj new_obj("zone.domain_root"/*store->zone.domain_root*/, obj.bucket.name);
//    objs_state[new_obj].is_atomic = true;
  }
}

void RGWObjectCtx::set_prefetch_data(rgw_obj& obj) {
  if (!obj.get_object().empty()) {
    objs_state[obj].prefetch_data = true;
  } else {
//    rgw_obj new_obj("zone.domain_root"/*store->zone.domain_root*/, obj.bucket.name);
//    objs_state[new_obj].prefetch_data = true;
  }
}


