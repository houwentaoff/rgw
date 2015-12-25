/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rados.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 15:11:02
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#ifndef __PORTING_RADOS_H__
#define __PORTING_RADOS_H__

#include "porting_common.h"
#include "cls/user/cls_user_types.h"

class RGWRados;
struct RGWObjState;

struct RGWObjectCtx {
  RGWRados *store;
  map<rgw_obj, RGWObjState> objs_state;
  void *user_ctx;

  RGWObjectCtx(RGWRados *_store) : store(_store), user_ctx(NULL) { }
  RGWObjectCtx(RGWRados *_store, void *_user_ctx) : store(_store), user_ctx(_user_ctx) { }

  RGWObjState *get_state(rgw_obj& obj);
  void set_atomic(rgw_obj& obj);
  void set_prefetch_data(rgw_obj& obj);
  void invalidate(rgw_obj& obj);
};

class RGWRados
{
    public:
        RGWRados(){}
        ~RGWRados(){}
    public:
      int cls_user_list_buckets(rgw_obj& obj,
                            const string& in_marker, int max_entries,
                            list<cls_user_bucket_entry>& entries,
                            string *out_marker, bool *truncated);
      int get_bucket_entrypoint_info(RGWObjectCtx& obj_ctx, const string& bucket_name, RGWBucketEntryPoint& entry_point, RGWObjVersionTracker *objv_tracker, time_t *pmtime,
                                 map<string, bufferlist> *pattrs, rgw_cache_entry_info *cache_info = NULL);
      
      void set_atomic(void *ctx, rgw_obj& obj) {
        RGWObjectCtx *rctx = static_cast<RGWObjectCtx *>(ctx);
        rctx->set_atomic(obj);
      }
      void set_prefetch_data(void *ctx, rgw_obj& obj) {
        RGWObjectCtx *rctx = static_cast<RGWObjectCtx *>(ctx);
        rctx->set_prefetch_data(obj);
      }
      virtual int get_bucket_info(RGWObjectCtx& obj_ctx, const string& bucket_name, RGWBucketInfo& info,
                                  time_t *pmtime, map<string, bufferlist> *pattrs = NULL);
      
};

struct RGWObjState {
  rgw_obj obj;
  bool is_atomic;
  bool has_attrs;
  bool exists;
  uint64_t size;
  time_t mtime;
  uint64_t epoch;
  bufferlist obj_tag;
  string write_tag;
  bool fake_tag;
//  RGWObjManifest manifest;
  bool has_manifest;
  string shadow_obj;
  bool has_data;
  bufferlist data;
  bool prefetch_data;
  bool keep_tail;
  bool is_olh;
  bufferlist olh_tag;

  /* important! don't forget to update copy constructor */
  RGWObjVersionTracker objv_tracker;
  
  map<string, bufferlist> attrset;
  RGWObjState() : is_atomic(false), has_attrs(0), exists(false),
                  size(0), mtime(0), epoch(0), fake_tag(false), has_manifest(false),
                  has_data(false), prefetch_data(false), keep_tail(false), is_olh(false) {}
  RGWObjState(const RGWObjState& rhs) {
    obj = rhs.obj;
    is_atomic = rhs.is_atomic;
    has_attrs = rhs.has_attrs;
    exists = rhs.exists;
    size = rhs.size;
    mtime = rhs.mtime;
    epoch = rhs.epoch;
    if (rhs.obj_tag.length()) {
      obj_tag = rhs.obj_tag;
    }
    write_tag = rhs.write_tag;
    fake_tag = rhs.fake_tag;
    if (rhs.has_manifest) {
//      manifest = rhs.manifest;
    }
    has_manifest = rhs.has_manifest;
    shadow_obj = rhs.shadow_obj;
    has_data = rhs.has_data;
    if (rhs.data.length()) {
      data = rhs.data;
    }
    prefetch_data = rhs.prefetch_data;
    keep_tail = rhs.keep_tail;
    is_olh = rhs.is_olh;
    objv_tracker = rhs.objv_tracker;
  }

  bool get_attr(string name, bufferlist& dest) {
    map<string, bufferlist>::iterator iter = attrset.find(name);
    if (iter != attrset.end()) {
      dest = iter->second;
      return true;
    }
    return false;
  }

  void clear() {
    has_attrs = false;
    exists = false;
    fake_tag = false;
    epoch = 0;
    size = 0;
    mtime = 0;
    obj_tag.clear();
    shadow_obj.clear();
    attrset.clear();
    data.clear();
  }
};


#endif
