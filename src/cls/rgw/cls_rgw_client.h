#ifndef CEPH_CLS_RGW_CLIENT_H
#define CEPH_CLS_RGW_CLIENT_H

#include "include/types.h"
#include "include/str_list.h"
#include "include/rados/librados.hh"
#include "cls_rgw_types.h"
#include "cls_rgw_ops.h"
//#include "common/RefCountedObj.h"
#include "include/compat.h"

// Forward declaration
class BucketIndexAioManager;

class BucketIndexAioManager {
};
/* bucket index */
//void cls_rgw_bucket_init(librados::ObjectWriteOperation& o);

class CLSRGWConcurrentIO {
protected:
  librados::IoCtx& io_ctx;
  map<int, string>& objs_container;
  map<int, string>::iterator iter;
  uint32_t max_aio;
//  BucketIndexAioManager manager;

  virtual int issue_op(int shard_id, const string& oid) = 0;

  virtual void cleanup() {}
  virtual int valid_ret_code() { return 0; }
  // Return true if multiple rounds of OPs might be needed, this happens when
  // OP needs to be re-send until a certain code is returned.
  virtual bool need_multiple_rounds() { return false; }
  // Add a new object to the end of the container.
  virtual void add_object(int shard, const string& oid) {}
  virtual void reset_container(map<int, string>& objs) {}

public:
  CLSRGWConcurrentIO(librados::IoCtx& ioc, map<int, string>& _objs_container,
                     uint32_t _max_aio) : io_ctx(ioc), objs_container(_objs_container), max_aio(_max_aio) {}
  virtual ~CLSRGWConcurrentIO() {}

  int operator()() {
    int ret = 0;
    iter = objs_container.begin();
    for (; iter != objs_container.end() && max_aio-- > 0; ++iter) {
      ret = issue_op(iter->first, iter->second);
      if (ret < 0)
        break;
    }

    int num_completions, r = 0;
    map<int, string> objs;
    map<int, string> *pobjs = (need_multiple_rounds() ? &objs : NULL);
//    while (manager.wait_for_completions(valid_ret_code(), &num_completions, &r, pobjs)) {
      if (r >= 0 && ret >= 0) {
        for(int i = 0; /*i < num_completions &&*/ iter != objs_container.end(); ++i, ++iter) {
          int issue_ret = issue_op(iter->first, iter->second);
          if(issue_ret < 0) {
            ret = issue_ret;
            break;
          }
        }
      } else if (ret >= 0) {
        ret = r;
      }
      if (need_multiple_rounds() && iter == objs_container.end() && !objs.empty()) {
        // For those objects which need another round, use them to reset
        // the container
        reset_container(objs);
      }
//    }

    if (ret < 0) {
      cleanup();
    }
    return ret;
  }
};

class CLSRGWIssueBucketIndexInit : public CLSRGWConcurrentIO {
protected:
  int issue_op(int shard_id, const string& oid);
  int valid_ret_code() { return -EEXIST; }
  void cleanup(){};
public:
  CLSRGWIssueBucketIndexInit(librados::IoCtx& ioc, map<int, string>& _bucket_objs,
                     uint32_t _max_aio) :
    CLSRGWConcurrentIO(ioc, _bucket_objs, _max_aio) {}
};

class CLSRGWIssueSetTagTimeout : public CLSRGWConcurrentIO {
  uint64_t tag_timeout;
protected:
  int issue_op(int shard_id, const string& oid){};
public:
  CLSRGWIssueSetTagTimeout(librados::IoCtx& ioc, map<int, string>& _bucket_objs,
                     uint32_t _max_aio, uint64_t _tag_timeout) :
    CLSRGWConcurrentIO(ioc, _bucket_objs, _max_aio), tag_timeout(_tag_timeout) {}
};
#if 0
void cls_rgw_bucket_prepare_op(librados::ObjectWriteOperation& o, RGWModifyOp op, string& tag,
                               const cls_rgw_obj_key& key, const string& locator, bool log_op,
                               uint16_t bilog_op);

void cls_rgw_bucket_complete_op(librados::ObjectWriteOperation& o, RGWModifyOp op, string& tag,
                                rgw_bucket_entry_ver& ver,
                                const cls_rgw_obj_key& key,
                                rgw_bucket_dir_entry_meta& dir_meta,
				list<cls_rgw_obj_key> *remove_objs, bool log_op,
                                uint16_t bilog_op);

void cls_rgw_remove_obj(librados::ObjectWriteOperation& o, list<string>& keep_attr_prefixes);
void cls_rgw_obj_check_attrs_prefix(librados::ObjectOperation& o, const string& prefix, bool fail_if_exist);

int cls_rgw_bi_get(librados::IoCtx& io_ctx, const string oid,
                   BIIndexType index_type, cls_rgw_obj_key& key,
                   rgw_cls_bi_entry *entry);
int cls_rgw_bi_put(librados::IoCtx& io_ctx, const string oid, rgw_cls_bi_entry& entry);
int cls_rgw_bi_list(librados::IoCtx& io_ctx, const string oid,
                   const string& name, const string& marker, uint32_t max,
                   list<rgw_cls_bi_entry> *entries, bool *is_truncated);


int cls_rgw_bucket_link_olh(librados::IoCtx& io_ctx, const string& oid, const cls_rgw_obj_key& key, bufferlist& olh_tag,
                            bool delete_marker, const string& op_tag, struct rgw_bucket_dir_entry_meta *meta,
                            uint64_t olh_epoch, bool log_op);
int cls_rgw_bucket_unlink_instance(librados::IoCtx& io_ctx, const string& oid, const cls_rgw_obj_key& key, const string& op_tag,
                                   uint64_t olh_epoch, bool log_op);
int cls_rgw_get_olh_log(librados::IoCtx& io_ctx, string& oid, librados::ObjectReadOperation& op, const cls_rgw_obj_key& olh, uint64_t ver_marker,
                        const string& olh_tag,
                        map<uint64_t, vector<struct rgw_bucket_olh_log_entry> > *log, bool *is_truncated);
void cls_rgw_trim_olh_log(librados::ObjectWriteOperation& op, const cls_rgw_obj_key& olh, uint64_t ver, const string& olh_tag);
int cls_rgw_clear_olh(librados::IoCtx& io_ctx, string& oid, const cls_rgw_obj_key& olh, const string& olh_tag);
#endif
/**
 * List the bucket with the starting object and filter prefix.
 * NOTE: this method do listing requests for each bucket index shards identified by
 *       the keys of the *list_results* map, which means the map should be popludated
 *       by the caller to fill with each bucket index object id.
 *
 * io_ctx        - IO context for rados.
 * start_obj     - marker for the listing.
 * filter_prefix - filter prefix.
 * num_entries   - number of entries to request for each object (note the total
 *                 amount of entries returned depends on the number of shardings).
 * list_results  - the list results keyed by bucket index object id.
 * max_aio       - the maximum number of AIO (for throttling).
 *
 * Return 0 on success, a failure code otherwise.
*/

class CLSRGWIssueBucketList : public CLSRGWConcurrentIO {
  cls_rgw_obj_key start_obj;
  string filter_prefix;
  uint32_t num_entries;
  bool list_versions;
  map<int, rgw_cls_list_ret>& result;
protected:
  int issue_op(int shard_id, const string& oid);
public:
  CLSRGWIssueBucketList(librados::IoCtx& io_ctx, const cls_rgw_obj_key& _start_obj,
                        const string& _filter_prefix, uint32_t _num_entries,
                        bool _list_versions,
                        map<int, string>& oids,
                        map<int, struct rgw_cls_list_ret>& list_results,
                        uint32_t max_aio) :
  CLSRGWConcurrentIO(io_ctx, oids, max_aio),
  start_obj(_start_obj), filter_prefix(_filter_prefix), num_entries(_num_entries), list_versions(_list_versions), result(list_results) {}
};

#if 0
int cls_rgw_get_dir_header_async(librados::IoCtx& io_ctx, string& oid, RGWGetDirHeader_CB *ctx);

void cls_rgw_encode_suggestion(char op, rgw_bucket_dir_entry& dirent, bufferlist& updates);

void cls_rgw_suggest_changes(librados::ObjectWriteOperation& o, bufferlist& updates);

/* usage logging */
int cls_rgw_usage_log_read(librados::IoCtx& io_ctx, string& oid, string& user,
                           uint64_t start_epoch, uint64_t end_epoch, uint32_t max_entries,
                           string& read_iter, map<rgw_user_bucket, rgw_usage_log_entry>& usage,
                           bool *is_truncated);

void cls_rgw_usage_log_trim(librados::ObjectWriteOperation& op, string& user,
                           uint64_t start_epoch, uint64_t end_epoch);

void cls_rgw_usage_log_add(librados::ObjectWriteOperation& op, rgw_usage_log_info& info);

/* garbage collection */
void cls_rgw_gc_set_entry(librados::ObjectWriteOperation& op, uint32_t expiration_secs, cls_rgw_gc_obj_info& info);
void cls_rgw_gc_defer_entry(librados::ObjectWriteOperation& op, uint32_t expiration_secs, const string& tag);

int cls_rgw_gc_list(librados::IoCtx& io_ctx, string& oid, string& marker, uint32_t max, bool expired_only,
                    list<cls_rgw_gc_obj_info>& entries, bool *truncated);

void cls_rgw_gc_remove(librados::ObjectWriteOperation& op, const list<string>& tags);
#endif
#endif
