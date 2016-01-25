/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_op.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/11 14:25:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __PORTING_OP_H__
#define __PORTING_OP_H__

#include "include/porting.h"
#include "porting_common.h"
#include "rgw_client_io.h"
#include "porting_bucket.h"

struct req_state;
class RGWHandler;

enum RGWOpType {
  RGW_OP_UNKNOWN = 0,
  RGW_OP_GET_OBJ,
  RGW_OP_LIST_BUCKETS,
  RGW_OP_STAT_ACCOUNT,
  RGW_OP_LIST_BUCKET,
  RGW_OP_GET_BUCKET_LOGGING,
  RGW_OP_GET_BUCKET_VERSIONING,
  RGW_OP_SET_BUCKET_VERSIONING,
  RGW_OP_STAT_BUCKET,
  RGW_OP_CREATE_BUCKET,
  RGW_OP_DELETE_BUCKET,
  RGW_OP_PUT_OBJ,
  RGW_OP_POST_OBJ,
  RGW_OP_PUT_METADATA_ACCOUNT,
  RGW_OP_PUT_METADATA_BUCKET,
  RGW_OP_PUT_METADATA_OBJECT,
  RGW_OP_SET_TEMPURL,
  RGW_OP_DELETE_OBJ,
  RGW_OP_COPY_OBJ,
  RGW_OP_GET_ACLS,
  RGW_OP_PUT_ACLS,
  RGW_OP_GET_CORS,
  RGW_OP_PUT_CORS,
  RGW_OP_DELETE_CORS,
  RGW_OP_OPTIONS_CORS,
  RGW_OP_GET_REQUEST_PAYMENT,
  RGW_OP_SET_REQUEST_PAYMENT,
  RGW_OP_INIT_MULTIPART,
  RGW_OP_COMPLETE_MULTIPART,
  RGW_OP_ABORT_MULTIPART,
  RGW_OP_LIST_MULTIPART,
  RGW_OP_LIST_BUCKET_MULTIPARTS,
  RGW_OP_DELETE_MULTI_OBJ,
};
/**
 * Provide the base class for all ops.
 */
class RGWOp {
protected:
  struct req_state *s;
  RGWHandler *dialect_handler;
  RGWRados *store;
//  RGWCORSConfiguration bucket_cors;
  bool cors_exist;
  RGWQuotaInfo bucket_quota;
  RGWQuotaInfo user_quota;

  virtual int init_quota();
public:
  RGWOp() : s(NULL), dialect_handler(NULL), store(NULL), cors_exist(false) {}
  virtual ~RGWOp() {}

  virtual int init_processing() {
    int ret = init_quota();
    if (ret < 0)
      return ret;

    return 0;
  }

  virtual void init(RGWRados *store, struct req_state *s, RGWHandler *dialect_handler) {
    this->store = store;
    this->s = s;
    this->dialect_handler = dialect_handler;
  }
  int read_bucket_cors();
  bool generate_cors_headers(string& origin, string& method, string& headers, string& exp_headers, unsigned *max_age);

  virtual int verify_params() { return 0; }
  virtual bool prefetch_data() { return false; }
  virtual int verify_permission() = 0;
  virtual int verify_op_mask();
  virtual void pre_exec() {}
  virtual void execute() = 0;
  virtual void send_response() {}
  virtual void complete() {
    send_response();
  }
  virtual const string name() = 0;
  virtual RGWOpType get_type() { return RGW_OP_UNKNOWN; }

  virtual uint32_t op_mask() { return 0; }
};


class RGWHandler {
protected:
  RGWRados *store;
  struct req_state *s;

  int do_read_permissions(RGWOp *op, bool only_bucket);

  virtual RGWOp *op_get() { return NULL; }
  virtual RGWOp *op_put() { return NULL; }
  virtual RGWOp *op_delete() { return NULL; }
  virtual RGWOp *op_head() { return NULL; }
  virtual RGWOp *op_post() { return NULL; }
  virtual RGWOp *op_copy() { return NULL; }
  virtual RGWOp *op_options() { return NULL; }
public:
  RGWHandler() : store(NULL), s(NULL) {}
  virtual ~RGWHandler();
  virtual int init(RGWRados *store, struct req_state *_s, RGWClientIO *cio);

  virtual RGWOp *get_op(RGWRados *store);
  virtual void put_op(RGWOp *op);
  virtual int read_permissions(RGWOp *op) = 0;
  virtual int authorize() = 0;
};
#define RGW_LIST_BUCKETS_LIMIT_MAX 10000

class RGWListBuckets : public RGWOp {
protected:
  int ret;
  bool sent_data;
  string marker;
  int64_t limit;
  uint64_t limit_max;
  uint32_t buckets_count;
  uint64_t buckets_objcount;
  uint64_t buckets_size;
  uint64_t buckets_size_rounded;
  map<string, bufferlist> attrs;

public:
  RGWListBuckets() : ret(0), sent_data(false) {
    limit = limit_max = RGW_LIST_BUCKETS_LIMIT_MAX;
    buckets_count = 0;
    buckets_objcount = 0;
    buckets_size = 0;
    buckets_size_rounded = 0;
  }

  int verify_permission();
  void execute();

  virtual int get_params() = 0;
  virtual void send_response_begin(bool has_buckets) = 0;
  virtual void send_response_data(RGWUserBuckets& buckets) = 0;
  virtual void send_response_end() = 0;
  virtual void send_response() {}

  virtual bool should_get_stats() { return false; }
  virtual bool supports_account_metadata() { return false; }

  virtual const string name() { return "list_buckets"; }
  virtual RGWOpType get_type() { return RGW_OP_LIST_BUCKETS; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_READ; }
};
class RGWListBucket : public RGWOp {
protected:
  RGWBucketEnt bucket;
  string prefix;
  rgw_obj_key marker; 
  rgw_obj_key next_marker; 
  rgw_obj_key end_marker;
  string max_keys;
  string delimiter;
  string encoding_type;
  bool list_versions;
  int max;
  int ret;
  vector<RGWObjEnt> objs;
  map<string, bool> common_prefixes;

  int default_max;
  bool is_truncated;

  int parse_max_keys();

public:
  RGWListBucket() : list_versions(false), max(0), ret(0),
                    default_max(0), is_truncated(false) {}
  int verify_permission();
  void pre_exec();
  void execute();

  virtual int get_params() = 0;
  virtual void send_response() = 0;
  virtual const string name() { return "list_bucket"; }
  virtual RGWOpType get_type() { return RGW_OP_LIST_BUCKET; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_READ; }
  virtual bool need_container_stats() { return false; }//just swift need
};

class RGWGetObj : public RGWOp {
protected:
  const char *range_str;
  const char *if_mod;
  const char *if_unmod;
  const char *if_match;
  const char *if_nomatch;
  off_t ofs;
  uint64_t total_len;
  off_t start;
  off_t end;
  time_t mod_time;
  time_t lastmod;
  time_t unmod_time;
  time_t *mod_ptr;
  time_t *unmod_ptr;
  map<string, bufferlist> attrs;
  int ret;
  bool get_data;
  bool partial_content;
  bool range_parsed;
  bool skip_manifest;
  rgw_obj obj;
  utime_t gc_invalidate_time;

  int init_common();
public:
  RGWGetObj() {
    range_str = NULL;
    if_mod = NULL;
    if_unmod = NULL;
    if_match = NULL;
    if_nomatch = NULL;
    start = 0;
    ofs = 0;
    total_len = 0;
    end = -1;
    mod_time = 0;
    lastmod = 0;
    unmod_time = 0;
    mod_ptr = NULL;
    unmod_ptr = NULL;
    get_data = false;
    partial_content = false;
    range_parsed = false;
    skip_manifest = false;
    ret = 0;
 }

  bool prefetch_data();

  void set_get_data(bool get_data) {
    this->get_data = get_data;
  }
  int verify_permission();
  void pre_exec();
  void execute();
  int read_user_manifest_part(rgw_bucket& bucket, RGWObjEnt& ent, RGWAccessControlPolicy *bucket_policy, off_t start_ofs, off_t end_ofs);
  int handle_user_manifest(const char *prefix);

  int get_data_cb(bufferlist& bl, off_t ofs, off_t len);

  virtual int get_params() = 0;
  virtual int send_response_data_error() = 0;
  virtual int send_response_data(bufferlist& bl, off_t ofs, off_t len) = 0;

  virtual const string name() { return "get_obj"; }
  virtual RGWOpType get_type() { return RGW_OP_GET_OBJ; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_READ; }
  virtual bool need_object_expiration() { return false; }
};

class RGWCreateBucket : public RGWOp {
protected:
  int ret;
  RGWAccessControlPolicy policy;
  string location_constraint;
  string placement_rule;
  RGWBucketInfo info;
  obj_version ep_objv;
  bool has_cors;
//  RGWCORSConfiguration cors_config;

  bufferlist in_data;

public:
  RGWCreateBucket() : ret(0), has_cors(false) {}

  int verify_permission(){};
  void pre_exec();
  void execute();
  virtual void init(RGWRados *store, struct req_state *s, RGWHandler *h) {
    RGWOp::init(store, s, h);
    policy.set_ctx(s->cct);
  }
  virtual int get_params() { return 0; }
  virtual void send_response() = 0;
  virtual const string name() { return "create_bucket"; }
  virtual RGWOpType get_type() { return RGW_OP_CREATE_BUCKET; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_WRITE; }
};

class RGWDeleteBucket : public RGWOp {
protected:
  int ret;

  RGWObjVersionTracker objv_tracker;

public:
  RGWDeleteBucket() : ret(0) {}

  int verify_permission();
  void pre_exec();
  void execute();

  virtual void send_response() = 0;
  virtual const string name() { return "delete_bucket"; }
  virtual RGWOpType get_type() { return RGW_OP_DELETE_BUCKET; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_DELETE; }
};

class RGWPutObj : public RGWOp {

  friend class RGWPutObjProcessor;

protected:
  int ret;
  off_t ofs;
  const char *supplied_md5_b64;
  const char *supplied_etag;
  const char *if_match;
  const char *if_nomatch;
  string etag;
  bool chunked_upload;
//  RGWAccessControlPolicy policy;
  const char *obj_manifest;
  time_t mtime;

//  MD5 *user_manifest_parts_hash;

  uint64_t olh_epoch;
  string version_id;

  time_t delete_at;

public:
  RGWPutObj() {
    ret = 0;
    ofs = 0;
    supplied_md5_b64 = NULL;
    supplied_etag = NULL;
    if_match = NULL;
    if_nomatch = NULL;
    chunked_upload = false;
    obj_manifest = NULL;
    mtime = 0;
//    user_manifest_parts_hash = NULL;
    olh_epoch = 0;
    delete_at = 0;
  }

  virtual void init(RGWRados *store, struct req_state *s, RGWHandler *h) {
    RGWOp::init(store, s, h);
//    policy.set_ctx(s->cct);
  }

//  RGWPutObjProcessor *select_processor(RGWObjectCtx& obj_ctx, bool *is_multipart){};
//  void dispose_processor(RGWPutObjProcessor *processor){};

  int verify_permission(){};
  void pre_exec();
  void execute();

  virtual int get_params() = 0;
  virtual int get_data(bufferlist& bl) = 0;
  virtual void send_response() = 0;
  virtual const string name() { return "put_obj"; }
  virtual RGWOpType get_type() { return RGW_OP_PUT_OBJ; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_WRITE; }
};
class RGWInitMultipart : public RGWOp {
protected:
  int ret;
  string upload_id;
  RGWAccessControlPolicy policy;

public:
#if 0
  off_t ofs;//add by sean
  int get_data(bufferlist& bl);//add by sean
#endif
  RGWInitMultipart() {
    ret = 0;
  }

  virtual void init(RGWRados *store, struct req_state *s, RGWHandler *h) {
    RGWOp::init(store, s, h);
    policy.set_ctx(s->cct);
  }
  int verify_permission();
  void pre_exec();
  void execute();

  virtual int get_params() = 0;
  virtual void send_response() = 0;
  virtual const string name() { return "init_multipart"; }
  virtual RGWOpType get_type() { return RGW_OP_INIT_MULTIPART; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_WRITE; }
};
#define MP_META_SUFFIX ".meta"

class RGWMPObj {
  string oid;
  string prefix;
  string meta;
  string upload_id;
public:
  RGWMPObj() {}
  RGWMPObj(const string& _oid, const string& _upload_id) {
    init(_oid, _upload_id, _upload_id);
  }
  void init(const string& _oid, const string& _upload_id) {
    init(_oid, _upload_id, _upload_id);
  }
  void init(const string& _oid, const string& _upload_id, const string& part_unique_str) {
    if (_oid.empty()) {
      clear();
      return;
    }
    oid = _oid;
    upload_id = _upload_id;
    prefix = oid + ".";
    meta = prefix + upload_id + MP_META_SUFFIX;
    prefix.append(part_unique_str);
  }
  string& get_meta() { return meta; }
  string get_part(int num) {
    char buf[16];
    snprintf(buf, 16, ".%d", num);
    string s = prefix;
    s.append(buf);
    return s;
  }
  string get_part(string& part) {
    string s = prefix;
    s.append(".");
    s.append(part);
    return s;
  }
  string& get_upload_id() {
    return upload_id;
  }
  string& get_key() {
    return oid;
  }
  bool from_meta(string& meta) {
    int end_pos = meta.rfind('.'); // search for ".meta"
    if (end_pos < 0)
      return false;
    int mid_pos = meta.rfind('.', end_pos - 1); // <key>.<upload_id>
    if (mid_pos < 0)
      return false;
    oid = meta.substr(0, mid_pos);
    upload_id = meta.substr(mid_pos + 1, end_pos - mid_pos - 1);
    init(oid, upload_id, upload_id);
    return true;
  }
  void clear() {
    oid = "";
    prefix = "";
    meta = "";
    upload_id = "";
  }
};
class RGWDeleteObj : public RGWOp {
protected:
  int ret;
  bool delete_marker;
  string version_id;

public:
  RGWDeleteObj() : ret(0), delete_marker(false) {}

  int verify_permission();
  void pre_exec();
  void execute();

  virtual void send_response() = 0;
  virtual const string name() { return "delete_obj"; }
  virtual RGWOpType get_type() { return RGW_OP_DELETE_OBJ; }
  virtual uint32_t op_mask() { return RGW_OP_TYPE_DELETE; }
  virtual bool need_object_expiration() { return false; }
};


#endif
