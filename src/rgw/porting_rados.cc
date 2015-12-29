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

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include "common/ceph_json.h"
#include "common/utf8.h"

#include "porting_rados.h"
#include "cls/user/cls_user_client.h"
#include "include/rados/librados.hh"
using namespace librados;

#include "porting_tools.h"
#include "common/dout.h"
#include "cls/rgw/cls_rgw_ops.h"
#include "cls/rgw/cls_rgw_client.h"
#include "include/porting.h"
#include "common/RefCountedObj.h"
#include "common/shell.h"
#include "global/global.h"

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <map>

#define MAX_BUCKET_INDEX_SHARDS_PRIME 7877

#define RGW_BUCKET_INSTANCE_MD_PREFIX ".bucket.meta."

using namespace std;

static string dir_oid_prefix = ".dir.";

#define dout_subsys ceph_subsys_rgw

struct bucket_info_entry {
  RGWBucketInfo info;
  time_t mtime;
  map<string, bufferlist> attrs;
};

static void filter_attrset(map<string, bufferlist>& unfiltered_attrset, const string& check_prefix,
                           map<string, bufferlist> *attrset)
{
  attrset->clear();
  map<string, bufferlist>::iterator iter;
  for (iter = unfiltered_attrset.lower_bound(check_prefix);
       iter != unfiltered_attrset.end(); ++iter) {
    if (!str_startswith(iter->first, check_prefix))
      break;
    (*attrset)[iter->first] = iter->second;
  }
}

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
 /* :TODO:Sunday, December 25, 2015 10:58:30 CST:hwt:  */
  rgw_bucket domain_root;
  char cmd_buf[256] = {0};

  sprintf(cmd_buf, "[ -d %s/%s ]", G.buckets_root.c_str(), bucket_name.c_str());
  int ret = shell_simple(cmd_buf);//rgw_get_system_obj(this, obj_ctx, domain_root/*zone.domain_root*/, bucket_name, bl, objv_tracker, pmtime, pattrs, cache_info);
  if (ret /*<*/ != 0) {
      ret = ret < 0 ? ret:0-ret;
    return ret;
  }
  entry_point.bucket.name = bucket_name;//add by sean
  entry_point.bucket.bucket_id = "bucket_id";//add by sean
  entry_point.bucket.oid = "oid";//add by sean
  entry_point.owner = "admin";//add by sean
//  entry_point.creation_time = ;//add by sean
  entry_point.encode(bl);
 /* :TODO:End---  */
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
  get_bucket_meta_oid(entry_point.bucket, oid);

  rgw_cache_entry_info cache_info;

  ret = get_bucket_instance_from_oid(obj_ctx, oid, e.info, &e.mtime, &e.attrs, &cache_info);
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

int RGWRados::SystemObject::Read::stat(RGWObjVersionTracker *objv_tracker)
{
  RGWRados *store = source->get_store();
  rgw_obj& obj = source->get_obj();

  return store->stat_system_obj(source->get_ctx(), state, obj, stat_params.attrs,
                                stat_params.lastmod, stat_params.obj_size, objv_tracker);
}

int RGWRados::stat_system_obj(RGWObjectCtx& obj_ctx,
                              RGWRados::SystemObject::Read::GetObjState& state,
                              rgw_obj& obj,
                              map<string, bufferlist> *attrs,
                              time_t *lastmod,
                              uint64_t *obj_size,
                              RGWObjVersionTracker *objv_tracker)
{
  RGWObjState *astate = NULL;

  int r = 0;//get_obj_state(&obj_ctx, obj, &astate, objv_tracker);
  if (r < 0)
    return r;
#if 0
  if (!astate->exists) {
    return -ENOENT;
  }

  if (attrs) {
    *attrs = astate->attrset;
//    if (cct->_conf->subsys.should_gather(ceph_subsys_rgw, 20)) {
      map<string, bufferlist>::iterator iter;
      for (iter = attrs->begin(); iter != attrs->end(); ++iter) {
        ldout(cct, 20) << "Read xattr: " << iter->first << dendl;
      }
//    }
  }

  if (obj_size)
    *obj_size = astate->size;
  if (lastmod)
    *lastmod = astate->mtime;
#endif
  return 0;
}
struct get_obj_data;

struct get_obj_aio_data {
  struct get_obj_data *op_data;
  off_t ofs;
  off_t len;
};

struct get_obj_io {
  off_t len;
  bufferlist bl;
};

struct get_obj_data : public RefCountedObject {
  CephContext *cct;
  RGWRados *rados;
  RGWObjectCtx *ctx;
  IoCtx io_ctx;
  map<off_t, get_obj_io> io_map;
  map<off_t, librados::AioCompletion *> completion_map;
  uint64_t total_read;
  Mutex lock;
  Mutex data_lock;
  list<get_obj_aio_data> aio_data;
  RGWGetDataCB *client_cb;
  atomic_t cancelled;
  atomic_t err_code;
  Throttle throttle;
  list<bufferlist> read_list;

  get_obj_data(CephContext *_cct)
    : cct(_cct),
      rados(NULL), ctx(NULL),
      total_read(0), lock("get_obj_data"), data_lock("get_obj_data::data_lock"),
      client_cb(NULL),
      throttle(cct, "get_obj_data", 10/* cct->_conf->rgw_get_obj_window_size */, false) {}
  virtual ~get_obj_data() { } 
  void set_cancelled(int r) {
    cancelled.set(1);
    err_code.set(r);
  }

  bool is_cancelled() {
    return cancelled.read() == 1;
  }

  int get_err_code() {
    return err_code.read();
  }

  int wait_next_io(bool *done) {
    lock.Lock();
    map<off_t, librados::AioCompletion *>::iterator iter = completion_map.begin();
    if (iter == completion_map.end()) {
      *done = true;
      lock.Unlock();
      return 0;
    }
    off_t cur_ofs = iter->first;
    librados::AioCompletion *c = iter->second;
    lock.Unlock();

    c->wait_for_complete_and_cb();
    int r = c->get_return_value();

    lock.Lock();
    completion_map.erase(cur_ofs);

    if (completion_map.empty()) {
      *done = true;
    }
    lock.Unlock();

    c->release();
    
    return r;
  }

  void add_io(off_t ofs, off_t len, bufferlist **pbl, AioCompletion **pc) {
    Mutex::Locker l(lock);

    get_obj_io& io = io_map[ofs];
    *pbl = &io.bl;

    struct get_obj_aio_data aio;
    aio.ofs = ofs;
    aio.len = len;
    aio.op_data = this;

    aio_data.push_back(aio);

    struct get_obj_aio_data *paio_data =  &aio_data.back(); /* last element */

    librados::AioCompletion *c = NULL;//librados::Rados::aio_create_completion((void *)paio_data, _get_obj_aio_completion_cb, NULL);
    completion_map[ofs] = c;

    *pc = c;

    /* we have a reference per IO, plus one reference for the calling function.
     * reference is dropped for each callback, plus when we're done iterating
     * over the parts */
    get();
  }

  void cancel_io(off_t ofs) {
    ldout(cct, 20) << "get_obj_data::cancel_io() ofs=" << ofs << dendl;
    lock.Lock();
    map<off_t, AioCompletion *>::iterator iter = completion_map.find(ofs);
    if (iter != completion_map.end()) {
      AioCompletion *c = iter->second;
      c->release();
      completion_map.erase(ofs);
      io_map.erase(ofs);
    }
    lock.Unlock();

    /* we don't drop a reference here -- e.g., not calling d->put(), because we still
     * need IoCtx to live, as io callback may still be called
     */
  }

  void cancel_all_io() {
    ldout(cct, 20) << "get_obj_data::cancel_all_io()" << dendl;
    Mutex::Locker l(lock);
    for (map<off_t, librados::AioCompletion *>::iterator iter = completion_map.begin();
         iter != completion_map.end(); ++iter) {
      librados::AioCompletion  *c = iter->second;
      c->release();
    }
  }

  int get_complete_ios(off_t ofs, list<bufferlist>& bl_list) {
    Mutex::Locker l(lock);

    map<off_t, get_obj_io>::iterator liter = io_map.begin();

    if (liter == io_map.end() ||
        liter->first != ofs) {
      return 0;
    }

    map<off_t, librados::AioCompletion *>::iterator aiter;
    aiter = completion_map.find(ofs);
    if (aiter == completion_map.end()) {
    /* completion map does not hold this io, it was cancelled */
      return 0;
    }

    AioCompletion *completion = aiter->second;
    int r = completion->get_return_value();
    if (r < 0)
      return r;

    for (; aiter != completion_map.end(); ++aiter) {
      completion = aiter->second;
      if (!completion->is_complete()) {
        /* reached a request that is not yet complete, stop */
        break;
      }

      r = completion->get_return_value();
      if (r < 0) {
        set_cancelled(r); /* mark it as cancelled, so that we don't continue processing next operations */
        return r;
      }

      total_read += r;

      map<off_t, get_obj_io>::iterator old_liter = liter++;
      bl_list.push_back(old_liter->second.bl);
      io_map.erase(old_liter);
    }

    return 0;
  }
};

static int _get_obj_iterate_cb(rgw_obj& obj, off_t obj_ofs, off_t read_ofs, off_t len, bool is_head_obj, RGWObjState *astate, void *arg)
{
  struct get_obj_data *d = (struct get_obj_data *)arg;

  return d->rados->get_obj_iterate_cb(d->ctx, astate, obj, obj_ofs, read_ofs, len, is_head_obj, arg);
}

int RGWRados::Object::Read::iterate(int64_t ofs, int64_t end, RGWGetDataCB *cb)
{
  RGWRados *store = source->get_store();
  CephContext *cct = NULL;//store->ctx();

  struct get_obj_data *data = new get_obj_data(cct);
  bool done = false;

  RGWObjectCtx& obj_ctx = source->get_ctx();

  data->rados = store;
  data->io_ctx.dup(state.io_ctx);
  data->client_cb = cb;

  int r = store->iterate_obj(obj_ctx, state.obj, ofs, end, 100/* cct->_conf->rgw_get_obj_max_req_size */, _get_obj_iterate_cb, (void *)data);
  if (r < 0) {
    data->cancel_all_io();
    goto done;
  }

  while (!done) {
    r = data->wait_next_io(&done);
    if (r < 0) {
      dout(10) << "get_obj_iterate() r=" << r << ", canceling all io" << dendl;
      data->cancel_all_io();
      break;
    }
    r = store->flush_read_list(data);
    if (r < 0) {
      dout(10) << "get_obj_iterate() r=" << r << ", canceling all io" << dendl;
      data->cancel_all_io();
      break;
    }
  }

done:
  data->put();
  return r;
}

/**
 * @brief 赋值bl
 *
 * @param ofs
 * @param end
 * @param bl
 *
 * @return 
 */
int RGWRados::Object::Read::read(int64_t ofs, int64_t end, bufferlist& bl)
{
  string test = "aabbccdd/ssff";
  bl.append(test);
  return bl.length();
}
int RGWRados::SystemObject::Read::read(int64_t ofs, int64_t end, bufferlist& bl, RGWObjVersionTracker *objv_tracker)
{
  RGWRados *store = source->get_store();
  rgw_obj& obj = source->get_obj();

  string test = "aabbccdd/ssff";
  bl.append(test);
  return bl.length();
//  return store->get_system_obj(source->get_ctx(), state, objv_tracker, obj, bl, ofs, end, read_params.cache_info);
}

/** 
 * get listing of the objects in a bucket.
 * bucket: bucket to list contents of
 * max: maximum number of results to return
 * prefix: only return results that match this prefix
 * delim: do not include results that match this string.
 *     Any skipped results will have the matching portion of their name
 *     inserted in common_prefixes with a "true" mark.
 * marker: if filled in, begin the listing with this object.
 * end_marker: if filled in, end the listing with this object.
 * result: the objects are put in here.
 * common_prefixes: if delim is filled in, any matching prefixes are placed
 *     here.
 */
int RGWRados::Bucket::List::list_objects(int max, vector<RGWObjEnt> *result,
                                         map<string, bool> *common_prefixes,
                                         bool *is_truncated)
{
  RGWRados *store = target->get_store();
  CephContext *cct = NULL;// store->ctx();
  rgw_bucket& bucket = target->get_bucket();

  int count = 0;
  bool truncated = true;

  if (store->bucket_is_system(bucket)) {
    return -EINVAL;
  }
  result->clear();

  rgw_obj marker_obj, end_marker_obj, prefix_obj;
  marker_obj.set_instance(params.marker.instance);
  marker_obj.set_ns(params.ns);
  marker_obj.set_obj(params.marker.name);
  rgw_obj_key cur_marker;
  marker_obj.get_index_key(&cur_marker);

  end_marker_obj.set_instance(params.end_marker.instance);
  end_marker_obj.set_ns(params.ns);
  end_marker_obj.set_obj(params.end_marker.name);
  rgw_obj_key cur_end_marker;
  if (params.ns.empty()) { /* no support for end marker for namespaced objects */
    end_marker_obj.get_index_key(&cur_end_marker);
  }
  const bool cur_end_marker_valid = !cur_end_marker.empty();

  prefix_obj.set_ns(params.ns);
  prefix_obj.set_obj(params.prefix);
  string cur_prefix = prefix_obj.get_index_key_name();

  string bigger_than_delim;

  if (!params.delim.empty()) {
    unsigned long val = decode_utf8((unsigned char *)params.delim.c_str(), params.delim.size());
    char buf[params.delim.size() + 16];
    int r = encode_utf8(val + 1, (unsigned char *)buf);
    if (r < 0) {
      ldout(cct,0) << "ERROR: encode_utf8() failed" << dendl;
      return -EINVAL;
    }
    buf[r] = '\0';

    bigger_than_delim = buf;
  }

  string skip_after_delim;

  /* if marker points at a common prefix, fast forward it into its upperbound string */
  if (!params.delim.empty()) {
    int delim_pos = cur_marker.name.find(params.delim, params.prefix.size());
    if (delim_pos >= 0) {
      string s = cur_marker.name.substr(0, delim_pos);
      s.append(bigger_than_delim);
      cur_marker.set(s);
    }
  }

  while (truncated && count <= max) {
    if (skip_after_delim > cur_marker.name) {
      cur_marker.set(skip_after_delim);
      ldout(cct, 20) << "setting cur_marker=" << cur_marker.name << "[" << cur_marker.instance << "]" << dendl;
    }
    std::map<string, RGWObjEnt> ent_map;
    int r = store->cls_bucket_list(bucket, cur_marker, cur_prefix, max + 1 - count, params.list_versions, ent_map,
                            &truncated, &cur_marker);
    if (r < 0)
      return r;

    std::map<string, RGWObjEnt>::iterator eiter;
    for (eiter = ent_map.begin(); eiter != ent_map.end(); ++eiter) {
      rgw_obj_key obj = eiter->second.key;
      RGWObjEnt& entry = eiter->second;
      rgw_obj_key key = obj;
      string instance;
      string ns;

      bool valid = rgw_obj::parse_raw_oid(obj.name, &obj.name, &instance, &ns);
      if (!valid) {
        ldout(cct, 0) << "ERROR: could not parse object name: " << obj.name << dendl;
        continue;
      }
      bool check_ns = (ns == params.ns);
      if (!params.list_versions && !entry.is_visible()) {
        continue;
      }

      if (params.enforce_ns && !check_ns) {
        if (!params.ns.empty()) {
          /* we've iterated past the namespace we're searching -- done now */
          truncated = false;
          goto done;
        }

        /* we're not looking at the namespace this object is in, next! */
        continue;
      }

      if (cur_end_marker_valid && cur_end_marker <= obj) {
        truncated = false;
        goto done;
      }

      if (count < max) {
        params.marker = obj;
        next_marker = obj;
      }

//      if (params.filter && !params.filter->filter(obj.name, key.name))
//        continue;

      if (params.prefix.size() &&  (obj.name.compare(0, params.prefix.size(), params.prefix) != 0))
        continue;

      if (!params.delim.empty()) {
        int delim_pos = obj.name.find(params.delim, params.prefix.size());

        if (delim_pos >= 0) {
          string prefix_key = obj.name.substr(0, delim_pos + 1);

          if (common_prefixes &&
              common_prefixes->find(prefix_key) == common_prefixes->end()) {
            if (count >= max) {
              truncated = true;
              goto done;
            }
            next_marker = prefix_key;
            (*common_prefixes)[prefix_key] = true;

            skip_after_delim = obj.name.substr(0, delim_pos);
            skip_after_delim.append(bigger_than_delim);

            ldout(cct, 20) << "skip_after_delim=" << skip_after_delim << dendl;

            count++;
          }

          continue;
        }
      }

      if (count >= max) {
        truncated = true;
        goto done;
      }

      RGWObjEnt ent = eiter->second;
      ent.key = obj;
      ent.ns = ns;
      result->push_back(ent);
      count++;
    }

    // Either the back-end telling us truncated, or we don't consume all
    // items returned per the amount caller request
    truncated = (truncated || eiter != ent_map.end());
  }

done:
  if (is_truncated)
    *is_truncated = truncated;

  return 0;
}
int RGWRados::cls_bucket_list(rgw_bucket& bucket, rgw_obj_key& start, const string& prefix,
		              uint32_t num_entries, bool list_versions, map<string, RGWObjEnt>& m,
			      bool *is_truncated, rgw_obj_key *last_entry,
			      bool (*force_check_filter)(const string&  name))
{
  ldout(cct, 10) << "cls_bucket_list " << bucket << " start " << start.name << "[" << start.instance << "] num_entries " << num_entries << dendl;

  librados::IoCtx index_ctx;
  // key   - oid (for different shards if there is any)
  // value - list result for the corresponding oid (shard), it is filled by the AIO callback
  map<int, string> oids;
  map<int, struct rgw_cls_list_ret> list_results;
  int r = open_bucket_index(bucket, index_ctx, oids);//ste value to oids
  if (r < 0)
    return r;

  cls_rgw_obj_key start_key(start.name, start.instance); 
  r = CLSRGWIssueBucketList(index_ctx, start_key, prefix, num_entries, list_versions,
                            oids, list_results,10 /*cct->_conf->rgw_bucket_index_max_aio*/)();//set oids to list_results
  if (r < 0)
    return r;

  // Create a list of iterators that are used to iterate each shard
  vector<map<string, struct rgw_bucket_dir_entry>::iterator> vcurrents(list_results.size());
  vector<map<string, struct rgw_bucket_dir_entry>::iterator> vends(list_results.size());
  vector<string> vnames(list_results.size());
  map<int, struct rgw_cls_list_ret>::iterator iter = list_results.begin();
  *is_truncated = false;
  for (; iter != list_results.end(); ++iter) {
    vcurrents.push_back(iter->second.dir.m.begin());
    vends.push_back(iter->second.dir.m.end());
    vnames.push_back(oids[iter->first]);
    *is_truncated = (*is_truncated || iter->second.is_truncated);
  }

  // Create a map to track the next candidate entry from each shard, if the entry
  // from a specified shard is selected/erased, the next entry from that shard will
  // be inserted for next round selection
  map<string, size_t> candidates;
  for (size_t i = 0; i < vcurrents.size(); ++i) {
    if (vcurrents[i] != vends[i]) {
      candidates[vcurrents[i]->first] = i;
    }
  }

  map<string, bufferlist> updates;
  uint32_t count = 0;
  while (count < num_entries && !candidates.empty()) {
    // Select the next one
    int pos = candidates.begin()->second;
    const string& name = vcurrents[pos]->first;
    struct rgw_bucket_dir_entry& dirent = vcurrents[pos]->second;

    // fill it in with initial values; we may correct later
    RGWObjEnt e;
    e.key.set(dirent.key.name, dirent.key.instance);
    e.size = dirent.meta.size;
    e.mtime = dirent.meta.mtime;
    e.etag = dirent.meta.etag;
    e.owner = dirent.meta.owner;
    e.owner_display_name = dirent.meta.owner_display_name;
    e.content_type = dirent.meta.content_type;
    e.tag = dirent.tag;
    e.flags = dirent.flags;
    e.versioned_epoch = dirent.versioned_epoch;
#if 0
    bool force_check = force_check_filter && force_check_filter(dirent.key.name);
    if ((!dirent.exists && !dirent.is_delete_marker()) || !dirent.pending_map.empty() || force_check) {
      /* there are uncommitted ops. We need to check the current state,
       * and if the tags are old we need to do cleanup as well. */
      librados::IoCtx sub_ctx;
      sub_ctx.dup(index_ctx);
      r = check_disk_state(sub_ctx, bucket, dirent, e, updates[vnames[pos]]);
      if (r < 0 && r != -ENOENT) {
          return r;
      }
    }
#endif
    if (r >= 0) {
      m[name] = e;
      ldout(cct, 10) << "RGWRados::cls_bucket_list: got " << e.key.name << "[" << e.key.instance << "]" << dendl;
      ++count;
    }

    // Refresh the candidates map
    candidates.erase(candidates.begin());
    ++vcurrents[pos];
    if (vcurrents[pos] != vends[pos]) {
      candidates[vcurrents[pos]->first] = pos;
    }
  }

  // Suggest updates if there is any
  map<string, bufferlist>::iterator miter = updates.begin();
  for (; miter != updates.end(); ++miter) {
#if 0
    if (miter->second.length()) {
      ObjectWriteOperation o;
      cls_rgw_suggest_changes(o, miter->second);
      // we don't care if we lose suggested updates, send them off blindly
      AioCompletion *c = librados::Rados::aio_create_completion(NULL, NULL, NULL);
      index_ctx.aio_operate(miter->first, c, &o);
        c->release();
    }
#endif
  }

  // Check if all the returned entries are consumed or not
  for (size_t i = 0; i < vcurrents.size(); ++i) {
    if (vcurrents[i] != vends[i])
      *is_truncated = true;
  }
  if (!m.empty())
    *last_entry = m.rbegin()->first;

  return 0;
}
int RGWRados::open_bucket_index(rgw_bucket& bucket, librados::IoCtx& index_ctx,
    map<int, string>& bucket_objs, int shard_id, map<int, string> *bucket_instance_ids) {
  string bucket_oid_base;
  int ret = open_bucket_index_base(bucket, index_ctx, bucket_oid_base);
  if (ret < 0)
    return ret;

  RGWObjectCtx obj_ctx(this);

  // Get the bucket info
  RGWBucketInfo binfo;
  ret = get_bucket_instance_info(obj_ctx, bucket, binfo, NULL, NULL);
  if (ret < 0)
    return ret;

  get_bucket_index_objects(bucket_oid_base, binfo.num_shards, bucket_objs, shard_id);
  if (bucket_instance_ids) {
    get_bucket_instance_ids(binfo, shard_id, bucket_instance_ids);
  }
  return 0;
}

template<typename T>
int RGWRados::open_bucket_index(rgw_bucket& bucket, librados::IoCtx& index_ctx,
                                map<int, string>& oids, map<int, T>& bucket_objs,
                                int shard_id, map<int, string> *bucket_instance_ids)
{
  int ret = open_bucket_index(bucket, index_ctx, oids, shard_id, bucket_instance_ids);
  if (ret < 0)
    return ret;

  map<int, string>::const_iterator iter = oids.begin();
  for (; iter != oids.end(); ++iter) {
    bucket_objs[iter->first] = T();
  }
  return 0;
}

int RGWRados::open_bucket_index_base(rgw_bucket& bucket, librados::IoCtx& index_ctx,
    string& bucket_oid_base) {
  if (bucket_is_system(bucket))
    return -EINVAL;

  int r = 0;//open_bucket_index_ctx(bucket, index_ctx);
  if (r < 0)
    return r;

//  if (bucket.marker.empty()) {//A beginning index for the list of objects returned. ??? sean
//    ldout(cct, 0) << "ERROR: empty marker for bucket operation" << dendl;
//    return -EIO;
//  }

  bucket_oid_base = dir_oid_prefix;
  bucket_oid_base.append(bucket.marker);

  return 0;

}

int RGWRados::get_bucket_instance_info(RGWObjectCtx& obj_ctx, rgw_bucket& bucket, RGWBucketInfo& info,
                                       time_t *pmtime, map<string, bufferlist> *pattrs)
{
  string oid;
  if (bucket.oid.empty()) {
    get_bucket_meta_oid(bucket, oid);
  } else {
    oid = bucket.oid;
  }

  return get_bucket_instance_from_oid(obj_ctx, oid, info, pmtime, pattrs);
}

int RGWRados::get_bucket_instance_from_oid(RGWObjectCtx& obj_ctx, string& oid, RGWBucketInfo& info,
                                           time_t *pmtime, map<string, bufferlist> *pattrs,
                                           rgw_cache_entry_info *cache_info)
{
//  ldout(cct, 20) << "reading from " << zone.domain_root << ":" << oid << dendl;

  bufferlist epbl;
 /* :TODO:Sunday, December 25, 2015 11:11:27 CST:hwt:  */
  rgw_bucket test;
  char cmd_buf[256] = {0};
  char bucket_name[256] = {0};
  char bucket_id[256] = {0};
//.bucket.meta.name:bucket.id
  sscanf(oid.c_str(), ".%*[^.].%*[^.].%[^:]:%s", bucket_name, bucket_id);
  sprintf(cmd_buf, "[ -d %s/%s ]", G.buckets_root.c_str(), bucket_name);
  
  int ret = shell_simple(cmd_buf);//rgw_get_system_obj(this, obj_ctx, test/*zone.domain_root*/, oid, epbl, &info.objv_tracker, pmtime, pattrs, cache_info);
  if (ret /*<*/ != 0) {
      ret = ret < 0 ? ret:0-ret;
    return ret;
  }
  info.bucket.name      = bucket_name;
  info.bucket.oid       = oid;
  info.bucket.bucket_id = bucket_name;//bucket_id;
  info.bucket.marker    = info.bucket.bucket_id;
  info.encode(epbl);//add by sean
 /* :TODO:End---  */
  bufferlist::iterator iter = epbl.begin();
  try {
    ::decode(info, iter);
  } catch (buffer::error& err) {
    ldout(cct, 0) << "ERROR: could not decode buffer info, caught buffer::error" << dendl;
    return -EIO;
  }
  info.bucket.oid = oid;
  return 0;
}
void RGWRados::get_bucket_index_objects(const string& bucket_oid_base,
    uint32_t num_shards, map<int, string>& bucket_objects, int shard_id)
{
  if (!num_shards) {
    bucket_objects[0] = bucket_oid_base;
  } else {
    char buf[bucket_oid_base.size() + 32];
    if (shard_id < 0) {
      for (uint32_t i = 0; i < num_shards; ++i) {
        snprintf(buf, sizeof(buf), "%s.%d", bucket_oid_base.c_str(), i);
        bucket_objects[i] = buf;
      }
    } else {
      if ((uint32_t)shard_id > num_shards) {
        return;
      }
      snprintf(buf, sizeof(buf), "%s.%d", bucket_oid_base.c_str(), shard_id);
      bucket_objects[shard_id] = buf;
    }
  }
}
void RGWRados::get_bucket_instance_ids(RGWBucketInfo& bucket_info, int shard_id, map<int, string> *result)
{
  rgw_bucket& bucket = bucket_info.bucket;
  string plain_id = bucket.name + ":" + bucket.bucket_id;
  if (!bucket_info.num_shards) {
    (*result)[0] = plain_id;
  } else {
    char buf[16];
    if (shard_id < 0) {
      for (uint32_t i = 0; i < bucket_info.num_shards; ++i) {
        snprintf(buf, sizeof(buf), ":%d", i);
        (*result)[i] = plain_id + buf;
      }
    } else {
      if ((uint32_t)shard_id > bucket_info.num_shards) {
        return;
      }
      snprintf(buf, sizeof(buf), ":%d", shard_id);
      (*result)[shard_id] = plain_id + buf;
    }
  }
}

void RGWRados::get_bucket_instance_entry(rgw_bucket& bucket, string& entry)
{
  entry = bucket.name + ":" + bucket.bucket_id;
}

void RGWRados::get_bucket_meta_oid(rgw_bucket& bucket, string& oid)
{
  string entry;
  get_bucket_instance_entry(bucket, entry);
  oid = RGW_BUCKET_INSTANCE_MD_PREFIX + entry;
}

/**
 * Get data about an object out of RADOS and into memory.
 * bucket: name of the bucket the object is in.
 * obj: name/key of the object to read
 * data: if get_data==true, this pointer will be set
 *    to an address containing the object's data/value
 * ofs: the offset of the object to read from
 * end: the point in the object to stop reading
 * attrs: if non-NULL, the pointed-to map will contain
 *    all the attrs of the object when this function returns
 * mod_ptr: if non-NULL, compares the object's mtime to *mod_ptr,
 *    and if mtime is smaller it fails.
 * unmod_ptr: if non-NULL, compares the object's mtime to *unmod_ptr,
 *    and if mtime is >= it fails.
 * if_match/nomatch: if non-NULL, compares the object's etag attr
 *    to the string and, if it doesn't/does match, fails out.
 * get_data: if true, the object's data/value will be read out, otherwise not
 * err: Many errors will result in this structure being filled
 *    with extra informatin on the error.
 * Returns: -ERR# on failure, otherwise
 *          (if get_data==true) length of read data,
 *          (if get_data==false) length of the object
 */
int RGWRados::Object::Read::prepare(int64_t *pofs, int64_t *pend)
{
  RGWRados *store = source->get_store();
  CephContext *cct = NULL;//store->ctx();

  bufferlist etag;

  off_t ofs = 0;
  off_t end = -1;

  map<string, bufferlist>::iterator iter;

  RGWObjState *astate;
  int r = source->get_state(&astate, true);
  if (r < 0)
    return r;

  if (!astate->exists) {
    return -ENOENT;
  }

  state.obj = astate->obj;

  r = 0;//store->get_obj_ioctx(state.obj, &state.io_ctx);
  if (r < 0) {
    return r;
  }

  if (params.attrs) {
    *params.attrs = astate->attrset;
//    if (cct->_conf->subsys.should_gather(ceph_subsys_rgw, 20)) {
//      for (iter = params.attrs->begin(); iter != params.attrs->end(); ++iter) {
//        ldout(cct, 20) << "Read xattr: " << iter->first << dendl;
//      }
//    }
  }

  /* Convert all times go GMT to make them compatible */
  if (conds.mod_ptr || conds.unmod_ptr) {
    time_t ctime = astate->mtime;

    if (conds.mod_ptr) {
      ldout(cct, 10) << "If-Modified-Since: " << *conds.mod_ptr << " Last-Modified: " << ctime << dendl;
      if (ctime < *conds.mod_ptr) {
        return -ERR_NOT_MODIFIED;
      }
    }

    if (conds.unmod_ptr) {
      ldout(cct, 10) << "If-UnModified-Since: " << *conds.unmod_ptr << " Last-Modified: " << ctime << dendl;
      if (ctime > *conds.unmod_ptr) {
        return -ERR_PRECONDITION_FAILED;
      }
    }
  }
  if (conds.if_match || conds.if_nomatch) {
    r = get_attr(RGW_ATTR_ETAG, etag);
    if (r < 0)
      return r;

    if (conds.if_match) {
      string if_match_str = rgw_string_unquote(conds.if_match);
      ldout(cct, 10) << "ETag: " << etag.c_str() << " " << " If-Match: " << if_match_str << dendl;
      if (if_match_str.compare(etag.c_str()) != 0) {
        return -ERR_PRECONDITION_FAILED;
      }
    }

    if (conds.if_nomatch) {
      string if_nomatch_str = rgw_string_unquote(conds.if_nomatch);
      ldout(cct, 10) << "ETag: " << etag.c_str() << " " << " If-NoMatch: " << if_nomatch_str << dendl;
      if (if_nomatch_str.compare(etag.c_str()) == 0) {
        return -ERR_NOT_MODIFIED;
      }
    }
  }

  if (pofs)
    ofs = *pofs;
  if (pend)
    end = *pend;

  if (ofs < 0) {
    ofs += astate->size;
    if (ofs < 0)
      ofs = 0;
    end = astate->size - 1;
  } else if (end < 0) {
    end = astate->size - 1;
  }

  if (astate->size > 0) {
    if (ofs >= (off_t)astate->size) {
      return -ERANGE;
    }
    if (end >= (off_t)astate->size) {
      end = astate->size - 1;
    }
  }

  if (pofs)
    *pofs = ofs;
  if (pend)
    *pend = end;
  if (params.read_size)
    *params.read_size = (ofs <= end ? end + 1 - ofs : 0);
  if (params.obj_size)
    *params.obj_size = astate->size;
  if (params.lastmod)
    *params.lastmod = astate->mtime;

  return 0;
}

int RGWRados::Object::get_state(RGWObjState **pstate, bool follow_olh)
{
  return store->get_obj_state(&ctx, obj, pstate, NULL, follow_olh);
}

int RGWRados::get_obj_state(RGWObjectCtx *rctx, rgw_obj& obj, RGWObjState **state, RGWObjVersionTracker *objv_tracker, bool follow_olh)
{
  int ret;

  do {
    ret = get_obj_state_impl(rctx, obj, state, objv_tracker, follow_olh);
  } while (ret == -EAGAIN);

  return ret;
}

static bool is_olh(map<string, bufferlist>& attrs)
{
  map<string, bufferlist>::iterator iter = attrs.find(RGW_ATTR_OLH_INFO);
  return (iter != attrs.end());
}

int RGWRados::get_obj_state_impl(RGWObjectCtx *rctx, rgw_obj& obj, RGWObjState **state, RGWObjVersionTracker *objv_tracker, bool follow_olh)
{
  bool need_follow_olh = follow_olh && !obj.have_instance();

  RGWObjState *s = rctx->get_state(obj);
//  ldout(cct, 20) << "get_obj_state: rctx=" << (void *)rctx << " obj=" << obj << " state=" << (void *)s << " s->prefetch_data=" << s->prefetch_data << dendl;
  *state = s;
  if (s->has_attrs) {
    if (s->is_olh && need_follow_olh) {
      return 0;//get_olh_target_state(*rctx, obj, s, state, objv_tracker);
    }
    return 0;
  }

  s->obj = obj;

  int r = raw_obj_stat(obj, &s->size, &s->mtime, &s->epoch, &s->attrset, (s->prefetch_data ? &s->data : NULL), objv_tracker);
  if (r == -ENOENT) {
    s->exists = false;
    s->has_attrs = true;
    s->mtime = 0;
    return 0;
  }
  if (r < 0)
    return r;

  s->exists = true;
  s->has_attrs = true;

  map<string, bufferlist>::iterator iter = s->attrset.find(RGW_ATTR_SHADOW_OBJ);
  if (iter != s->attrset.end()) {
    bufferlist bl = iter->second;
    bufferlist::iterator it = bl.begin();
    it.copy(bl.length(), s->shadow_obj);
    s->shadow_obj[bl.length()] = '\0';
  }
  s->obj_tag = s->attrset[RGW_ATTR_ID_TAG];
#if 0
  bufferlist manifest_bl = s->attrset[RGW_ATTR_MANIFEST];
  if (manifest_bl.length()) {
    bufferlist::iterator miter = manifest_bl.begin();
    try {
      ::decode(s->manifest, miter);
      s->has_manifest = true;
      s->size = s->manifest.get_obj_size();
    } catch (buffer::error& err) {
      ldout(cct, 20) << "ERROR: couldn't decode manifest" << dendl;
      return -EIO;
    }
    ldout(cct, 10) << "manifest: total_size = " << s->manifest.get_obj_size() << dendl;
    if (cct->_conf->subsys.should_gather(ceph_subsys_rgw, 20) && s->manifest.has_explicit_objs()) {
      RGWObjManifest::obj_iterator mi;
      for (mi = s->manifest.obj_begin(); mi != s->manifest.obj_end(); ++mi) {
        ldout(cct, 20) << "manifest: ofs=" << mi.get_ofs() << " loc=" << mi.get_location() << dendl;
      }
    }

    if (!s->obj_tag.length()) {
      /*
       * Uh oh, something's wrong, object with manifest should have tag. Let's
       * create one out of the manifest, would be unique
       */
      generate_fake_tag(cct, s->attrset, s->manifest, manifest_bl, s->obj_tag);
      s->fake_tag = true;
    }
  }
#endif
  if (s->obj_tag.length())
    ldout(cct, 20) << "get_obj_state: setting s->obj_tag to " << string(s->obj_tag.c_str(), s->obj_tag.length()) << dendl;
  else
    ldout(cct, 20) << "get_obj_state: s->obj_tag was set empty" << dendl;

  /* an object might not be olh yet, but could have olh id tag, so we should set it anyway if
   * it exist, and not only if is_olh() returns true
   */
  iter = s->attrset.find(RGW_ATTR_OLH_ID_TAG);
  if (iter != s->attrset.end()) {
    s->olh_tag = iter->second;
  }

  if (is_olh(s->attrset)) {
    s->is_olh = true;

    ldout(cct, 20) << __func__ << ": setting s->olh_tag to " << string(s->olh_tag.c_str(), s->olh_tag.length()) << dendl;

    if (need_follow_olh) {
      return 0;//get_olh_target_state(*rctx, obj, s, state, objv_tracker);
    }
  }

  return 0;
}


int RGWRados::raw_obj_stat(rgw_obj& obj, uint64_t *psize, time_t *pmtime, uint64_t *epoch,
                           map<string, bufferlist> *attrs, bufferlist *first_chunk,
                           RGWObjVersionTracker *objv_tracker)
{
  rgw_rados_ref ref;
  rgw_bucket bucket;
  int r = 0;//get_obj_ref(obj, &ref, &bucket);
  if (r < 0) {
    return r;
  }

  map<string, bufferlist> unfiltered_attrset;
  uint64_t size = 0;
  time_t mtime = 0;

  ObjectReadOperation op;
  if (objv_tracker) {
//    objv_tracker->prepare_op_for_read(&op);
  }
  if (attrs) {
    op.getxattrs(&unfiltered_attrset, NULL);
  }
  if (psize || pmtime) {
    op.stat(&size, &mtime, NULL);
  }
  if (first_chunk) {
    op.read(0, 100/* cct->_conf->rgw_max_chunk_size */, first_chunk, NULL);
  }
  bufferlist outbl;
  r = 0;//ref.ioctx.operate(ref.oid, &op, &outbl);

  if (epoch) {
//    *epoch = ref.ioctx.get_last_version();
  }

  if (r < 0)
    return r;

  if (psize)
    *psize = size;
  if (pmtime)
    *pmtime = mtime;
  if (attrs) {
    filter_attrset(unfiltered_attrset, RGW_ATTR_PREFIX, attrs);
  }

  return 0;
}

int RGWRados::Object::Read::get_attr(const char *name, bufferlist& dest)
{
  RGWObjState *state;
  int r = source->get_state(&state, true);
  if (r < 0)
    return r;
  if (!state->exists)
    return -ENOENT;
  if (!state->get_attr(name, dest))
    return -ENODATA;

  return 0;
}

int RGWRados::flush_read_list(struct get_obj_data *d)
{
  d->data_lock.Lock();
  list<bufferlist> l;
  l.swap(d->read_list);
  d->get();
  d->read_list.clear();

  d->data_lock.Unlock();

  int r = 0;

  list<bufferlist>::iterator iter;
  for (iter = l.begin(); iter != l.end(); ++iter) {
    bufferlist& bl = *iter;
    r = d->client_cb->handle_data(bl, 0, bl.length());
    if (r < 0) {
      dout(0) << "ERROR: flush_read_list(): d->client_c->handle_data() returned " << r << dendl;
      break;
    }
  }

  d->data_lock.Lock();
  d->put();
  if (r < 0) {
    d->set_cancelled(r);
  }
  d->data_lock.Unlock();
  return r;
}

