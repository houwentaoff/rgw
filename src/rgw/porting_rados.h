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

#include "common/RefCountedObj.h"
#include "common/RWLock.h"
#include "porting_common.h"

#include "cls/user/cls_user_types.h"
#include "porting_metadata.h"
#include "common/RWLock.h"

#include "include/rados/librados.hh"

#define RGW_BUCKET_INSTANCE_MD_PREFIX ".bucket.meta."

class RGWRados;
struct RGWObjState;

#define RGW_OBJ_NS_MULTIPART "multipart"
#define RGW_OBJ_NS_SHADOW    "shadow"

class RGWChainedCache {
public:
  virtual ~RGWChainedCache() {}
  virtual void chain_cb(const string& key, void *data) = 0;
  virtual void invalidate(const string& key) = 0;
  virtual void invalidate_all() = 0;

  struct Entry {
    RGWChainedCache *cache;
    const string& key;
    void *data;

    Entry(RGWChainedCache *_c, const string& _k, void *_d) : cache(_c), key(_k), data(_d) {}
  };
};


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

class RGWGetDataCB {
protected:
  uint64_t extra_data_len;
public:
  virtual int handle_data(bufferlist& bl, off_t bl_ofs, off_t bl_len) = 0;
  RGWGetDataCB() : extra_data_len(0) {}
  virtual ~RGWGetDataCB() {}
  virtual void set_extra_data_len(uint64_t len) {
    extra_data_len = len;
  }
};

class RGWRados
{
    public:
        RGWMetadataManager *meta_mgr;
        
    public:
        RGWRados():meta_mgr(NULL){  meta_mgr = new RGWMetadataManager(cct, this);}
        ~RGWRados(){}
    protected:
          CephContext *cct;
    public:
      int put_bucket_instance_info(RGWBucketInfo& info, bool exclusive, time_t mtime, map<string, bufferlist> *pattrs);

      int put_bucket_entrypoint_info(const string& bucket_name, RGWBucketEntryPoint& entry_point, bool exclusive, RGWObjVersionTracker& objv_tracker, time_t mtime,
                                     map<string, bufferlist> *pattrs);

      int cls_user_add_bucket(rgw_obj& obj, const cls_user_bucket_entry& entry);
      int cls_user_update_buckets(rgw_obj& obj, list<cls_user_bucket_entry>& entries, bool add);
      
      int iterate_obj(RGWObjectCtx& ctx, rgw_obj& obj,
                      off_t ofs, off_t end,
                      uint64_t max_chunk_size,
                      int (*iterate_obj_cb)(rgw_obj&, off_t, off_t, off_t, bool, RGWObjState *, void *),
                      void *arg){};        
      int flush_read_list(struct get_obj_data *d);      
      int get_obj_iterate_cb(RGWObjectCtx *ctx, RGWObjState *astate,
                             rgw_obj& obj,
                             off_t obj_ofs, off_t read_ofs, off_t len,
                             bool is_head_obj, void *arg){};
      
      int get_obj_state(RGWObjectCtx *rctx, rgw_obj& obj, RGWObjState **state, RGWObjVersionTracker *objv_tracker, bool follow_olh);

      int get_obj_state_impl(RGWObjectCtx *rctx, rgw_obj& obj, RGWObjState **state, RGWObjVersionTracker *objv_tracker, bool follow_olh);
       
      int cls_user_list_buckets(rgw_obj& obj,
                            const string& in_marker, int max_entries,
                            list<cls_user_bucket_entry>& entries,
                            string *out_marker, bool *truncated);
      int cls_bucket_list(rgw_bucket& bucket, rgw_obj_key& start, const string& prefix,
                          uint32_t num_entries, bool list_versions, map<string, RGWObjEnt>& m,
                          bool *is_truncated, rgw_obj_key *last_entry,
                          bool (*force_check_filter)(const string&  name) = NULL);
      
      int get_bucket_entrypoint_info(RGWObjectCtx& obj_ctx, const string& bucket_name, RGWBucketEntryPoint& entry_point, RGWObjVersionTracker *objv_tracker, time_t *pmtime,
                                 map<string, bufferlist> *pattrs, rgw_cache_entry_info *cache_info = NULL);
      void get_bucket_instance_entry(rgw_bucket& bucket, string& entry);
      void get_bucket_meta_oid(rgw_bucket& bucket, string& oid);
      
      void get_bucket_instance_ids(RGWBucketInfo& bucket_info, int shard_id, map<int, string> *result);
      
      int get_bucket_instance_info(RGWObjectCtx& obj_ctx, const string& meta_key, RGWBucketInfo& info, time_t *pmtime, map<string, bufferlist> *pattrs);

      int get_bucket_instance_info(RGWObjectCtx& obj_ctx, rgw_bucket& bucket, RGWBucketInfo& info, time_t *pmtime, map<string, bufferlist> *pattrs);
      int get_bucket_instance_from_oid(RGWObjectCtx& obj_ctx, string& oid, RGWBucketInfo& info, time_t *pmtime, map<string, bufferlist> *pattrs,
                                   rgw_cache_entry_info *cache_info = NULL);
      
      int open_bucket_index_base(rgw_bucket& bucket, librados::IoCtx&  index_ctx,
          string& bucket_oid_base);
      
      int open_bucket_index(rgw_bucket& bucket, librados::IoCtx& index_ctx,
                map<int, string>& bucket_objs, int shard_id = -1, map<int, string> *bucket_instance_ids = NULL);
      template<typename T>
      int open_bucket_index(rgw_bucket& bucket, librados::IoCtx& index_ctx,
                map<int, string>& oids, map<int, T>& bucket_objs,
                int shard_id = -1, map<int, string> *bucket_instance_ids = NULL);
      
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
  /**
   * a simple object read without keeping state
   */
      virtual int raw_obj_stat(rgw_obj& obj, uint64_t *psize, time_t *pmtime, uint64_t *epoch,
                               map<string, bufferlist> *attrs, bufferlist *first_chunk,
                               RGWObjVersionTracker *objv_tracker);
      virtual int create_bucket(RGWUserInfo& owner, rgw_bucket& bucket,
                                const string& region_name,
                                const string& placement_rule,
                                map<std::string,bufferlist>& attrs,
                                RGWBucketInfo& bucket_info,
                                obj_version *pobjv,
                                obj_version *pep_objv,
                                time_t creation_time,
                                rgw_bucket *master_bucket,
                                bool exclusive = true);

  class SystemObject {
    RGWRados *store;
    RGWObjectCtx& ctx;
    rgw_obj obj;

    RGWObjState *state;

  protected:
    int get_state(RGWObjState **pstate, RGWObjVersionTracker *objv_tracker);

  public:
    SystemObject(RGWRados *_store, RGWObjectCtx& _ctx, rgw_obj& _obj) : store(_store), ctx(_ctx), obj(_obj), state(NULL) {}

    RGWRados *get_store() { return store; }
    rgw_obj& get_obj() { return obj; }
    RGWObjectCtx& get_ctx() { return ctx; }
    struct Read {
      RGWRados::SystemObject *source;

      struct GetObjState {
        librados::IoCtx io_ctx;
        bool has_ioctx;
        uint64_t last_ver;

        GetObjState() : has_ioctx(false), last_ver(0) {}

        int get_ioctx(RGWRados *store, rgw_obj& obj, librados::IoCtx **ioctx);
      } state;
      struct StatParams {
        time_t *lastmod;
        uint64_t *obj_size;
        map<string, bufferlist> *attrs;
        struct rgw_err *perr;

        StatParams() : lastmod(NULL), obj_size(NULL), attrs(NULL), perr(NULL) {}
      } stat_params;

      struct ReadParams {
        rgw_cache_entry_info *cache_info;
      } read_params;
      
      Read(RGWRados::SystemObject *_source) : source(_source) {}

      int stat(RGWObjVersionTracker *objv_tracker);
      int read(int64_t ofs, int64_t end, bufferlist& bl, RGWObjVersionTracker *objv_tracker);
      int get_attr(const char *name, bufferlist& dest);
    };    
  };
  class Object {
    RGWRados *store;
    RGWBucketInfo bucket_info;
    RGWObjectCtx& ctx;
    rgw_obj obj;

//    BucketShard bs;

    RGWObjState *state;

    bool versioning_disabled;

    bool bs_initialized;
    protected:
        int get_state(RGWObjState **pstate, bool follow_olh);
    public:
        Object(RGWRados *_store, RGWBucketInfo& _bucket_info, RGWObjectCtx& _ctx, rgw_obj& _obj) : store(_store), bucket_info(_bucket_info),
                                                                                                   ctx(_ctx), obj(_obj)/*, bs(store)*/,
                                                                                                   state(NULL), versioning_disabled(false),
                                                                                                   bs_initialized(false) {}

        RGWRados *get_store() { return store; }
        rgw_obj& get_obj() { return obj; }
        RGWObjectCtx& get_ctx() { return ctx; }
        RGWBucketInfo& get_bucket_info() { return bucket_info; }
        void set_versioning_disabled(bool status) {
          versioning_disabled = status;
        }

        bool versioning_enabled() {
          return (!versioning_disabled && bucket_info.versioning_enabled());
        }
    struct Read {
      RGWRados::Object *source;

      struct GetObjState {
        librados::IoCtx io_ctx;
        rgw_obj obj;
      } state;
      
      struct ConditionParams {
        const time_t *mod_ptr;
        const time_t *unmod_ptr;
        const char *if_match;
        const char *if_nomatch;
        
        ConditionParams() : 
                 mod_ptr(NULL), unmod_ptr(NULL), if_match(NULL), if_nomatch(NULL) {}
      } conds;

      struct Params {
        time_t *lastmod;
        uint64_t *read_size;
        uint64_t *obj_size;
        map<string, bufferlist> *attrs;
        struct rgw_err *perr;

        Params() : lastmod(NULL), read_size(NULL), obj_size(NULL), attrs(NULL), perr(NULL) {}
      } params;

      Read(RGWRados::Object *_source) : source(_source) {}

      int prepare(int64_t *pofs, int64_t *pend);
      int read(int64_t ofs, int64_t end, bufferlist& bl);
      int iterate(int64_t ofs, int64_t end, RGWGetDataCB *cb);
      int get_attr(const char *name, bufferlist& dest);
    };       
  };      

  class Bucket
  {
    RGWRados *store;
    rgw_bucket& bucket;

  public:
    Bucket(RGWRados *_store, rgw_bucket& _bucket) : store(_store), bucket(_bucket) {}

    RGWRados *get_store() { return store; }
    rgw_bucket& get_bucket() { return bucket; }
    struct List {
      RGWRados::Bucket *target;
      rgw_obj_key next_marker;

      struct Params {
        string prefix;
        string delim;
        rgw_obj_key marker;
        rgw_obj_key end_marker;
        string ns;
        bool enforce_ns;
//        RGWAccessListFilter *filter;
        bool list_versions;

        Params() : enforce_ns(true)/*, filter(NULL)*/, list_versions(false) {}
      } params;

    public:
      List(RGWRados::Bucket *_target) : target(_target) {}

      int list_objects(int max, vector<RGWObjEnt> *result, map<string, bool> *common_prefixes, bool *is_truncated);
      rgw_obj_key& get_next_marker() {
        return next_marker;
      }
    };

  };

  virtual int stat_system_obj(RGWObjectCtx& obj_ctx,
                              RGWRados::SystemObject::Read::GetObjState& state,
                              rgw_obj& obj,
                              map<string, bufferlist> *attrs,
                              time_t *lastmod,
                              uint64_t *obj_size,
                              RGWObjVersionTracker *objv_tracker);
  virtual int put_linked_bucket_info(RGWBucketInfo& info,
                                     bool exclusive,
                                     time_t mtime,
                                     obj_version *pep_objv,
                                     map<string, bufferlist> *pattrs,
                                     bool create_entry_point);

  virtual void register_chained_cache(RGWChainedCache *cache) {}
  virtual bool chain_cache_entry(list<rgw_cache_entry_info *>& cache_info_entries, RGWChainedCache::Entry *chained_entry) { return false; }

  private:
  bool bucket_is_system(rgw_bucket& bucket) {
    return (bucket.name[0] == '.');
  }
  /**
   * This is a helper method, it generates a list of bucket index objects with the given
   * bucket base oid and number of shards.
   *
   * bucket_oid_base [in] - base name of the bucket index object;
   * num_shards [in] - number of bucket index object shards.
   * bucket_objs [out] - filled by this method, a list of bucket index objects.
   */
  void get_bucket_index_objects(const string& bucket_oid_base, uint32_t num_shards,
      map<int, string>& bucket_objs, int shard_id = -1);
      
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

struct rgw_rados_ref {
  string oid;
  string key;
  librados::IoCtx ioctx;
};

template <class T>
class RGWChainedCacheImpl : public RGWChainedCache {
  RWLock lock;

  map<string, T> entries;

public:
  RGWChainedCacheImpl() : lock("RGWChainedCacheImpl::lock") {}

  void init(RGWRados *store) {
    store->register_chained_cache(this);
  }

  bool find(const string& key, T *entry) {
    RWLock::RLocker rl(lock);
    typename map<string, T>::iterator iter = entries.find(key);
    if (iter == entries.end()) {
      return false;
    }

    *entry = iter->second;
    return true;
  }

  bool put(RGWRados *store, const string& key, T *entry, list<rgw_cache_entry_info *>& cache_info_entries) {
    Entry chain_entry(this, key, entry);

    /* we need the store cache to call us under its lock to maintain lock ordering */
    return store->chain_cache_entry(cache_info_entries, &chain_entry);
  }

  void chain_cb(const string& key, void *data) {
    T *entry = static_cast<T *>(data);
    RWLock::WLocker wl(lock);
    entries[key] = *entry;
  }

  void invalidate(const string& key) {
    RWLock::WLocker wl(lock);
    entries.erase(key);
  }

  void invalidate_all() {
    RWLock::WLocker wl(lock);
    entries.clear();
  }
};


#endif
