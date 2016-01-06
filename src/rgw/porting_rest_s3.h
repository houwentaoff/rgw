/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest_s3.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/14 11:28:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#ifndef __PORTING_REST_S3_H__
#define __PORTING_REST_S3_H__

#include "porting_rest.h"
#include "porting_op.h"

class RGWGetObj_ObjStore_S3 : public RGWGetObj_ObjStore
{
public:
  RGWGetObj_ObjStore_S3() {}
  ~RGWGetObj_ObjStore_S3() {}

  int send_response_data_error();
  int send_response_data(bufferlist& bl, off_t ofs, off_t len);
};

class RGW_Auth_S3 {
public:
  static int authorize(RGWRados *store, struct req_state *s);
};
class RGWHandler_Auth_S3 : public RGWHandler_ObjStore {
  friend class RGWRESTMgr_S3;
public:
  RGWHandler_Auth_S3() : RGWHandler_ObjStore() {}
  virtual ~RGWHandler_Auth_S3() {}

  virtual int validate_bucket_name(const string& bucket) {
    return 0;
  }

  virtual int validate_object_name(const string& bucket) { return 0; }

  virtual int init(RGWRados *store, struct req_state *state, RGWClientIO *cio);
  virtual int authorize() {
    return RGW_Auth_S3::authorize(store, s);
  }
};
class RGWHandler_ObjStore_S3 : public RGWHandler_ObjStore {
  friend class RGWRESTMgr_S3;
public:
  static int init_from_header(struct req_state *s, int default_formatter, bool configurable_format);

  RGWHandler_ObjStore_S3() : RGWHandler_ObjStore() {}
  virtual ~RGWHandler_ObjStore_S3() {}

  int validate_bucket_name(const string& bucket, bool relaxed_names);
  using RGWHandler_ObjStore::validate_bucket_name;
  
  virtual int init(RGWRados *store, struct req_state *state, RGWClientIO *cio);
  virtual int authorize() {
    return RGW_Auth_S3::authorize(store, s);
  }
};
class RGWHandler_ObjStore_Service_S3 : public RGWHandler_ObjStore_S3 {
protected:
  RGWOp *op_get();
  RGWOp *op_head();
public:
  RGWHandler_ObjStore_Service_S3() {}
  virtual ~RGWHandler_ObjStore_Service_S3() {}
};

class RGWHandler_ObjStore_Bucket_S3 : public RGWHandler_ObjStore_S3 {
protected:
  bool is_acl_op() {
    return s->info.args.exists("acl");
  }
  bool is_cors_op() {
      return s->info.args.exists("cors");
  }
  bool is_obj_update_op() {
    return is_acl_op() || is_cors_op();
  }
  bool is_request_payment_op() {
    return s->info.args.exists("requestPayment");
  }
  RGWOp *get_obj_op(bool get_data);

  RGWOp *op_get();
  RGWOp *op_head();
  RGWOp *op_put();
  RGWOp *op_delete();
  RGWOp *op_post();
  RGWOp *op_options();
public:
  RGWHandler_ObjStore_Bucket_S3() {}
  virtual ~RGWHandler_ObjStore_Bucket_S3() {}
};

class RGWRESTMgr_S3 : public RGWRESTMgr {
public:
  RGWRESTMgr_S3() {}
  virtual ~RGWRESTMgr_S3() {}

  virtual RGWHandler *get_handler(struct req_state *s);
};
class RGWHandler_ObjStore_Obj_S3 : public RGWHandler_ObjStore_S3 {
protected:
  bool is_acl_op() {
    return s->info.args.exists("acl");
  }
  bool is_cors_op() {
      return s->info.args.exists("cors");
  }
  bool is_obj_update_op() {
    return is_acl_op();
  }
  RGWOp *get_obj_op(bool get_data);

  RGWOp *op_get();
  RGWOp *op_head();
  RGWOp *op_put();
  RGWOp *op_delete();
  RGWOp *op_post();
  RGWOp *op_options();
public:
  RGWHandler_ObjStore_Obj_S3() {}
  virtual ~RGWHandler_ObjStore_Obj_S3() {}
};
class RGWListBuckets_ObjStore_S3 : public RGWListBuckets_ObjStore {
public:
  RGWListBuckets_ObjStore_S3() {}
  ~RGWListBuckets_ObjStore_S3() {}

  int get_params() {
    limit = -1; /* no limit */
    return 0;
  }
  virtual void send_response_begin(bool has_buckets);
  virtual void send_response_data(RGWUserBuckets& buckets);
  virtual void send_response_end();
};
class RGWListBucket_ObjStore_S3 : public RGWListBucket_ObjStore {
public:
  RGWListBucket_ObjStore_S3() {
    default_max = 1000;
  }
  ~RGWListBucket_ObjStore_S3() {}

  int get_params();
  void send_response();
  void send_versioned_response();
};
class RGWCreateBucket_ObjStore_S3 : public RGWCreateBucket_ObjStore {
public:
  RGWCreateBucket_ObjStore_S3() {}
  ~RGWCreateBucket_ObjStore_S3() {}

  int get_params();
  void send_response();
};
class RGWDeleteBucket_ObjStore_S3 : public RGWDeleteBucket_ObjStore {
public:
  RGWDeleteBucket_ObjStore_S3() {}
  ~RGWDeleteBucket_ObjStore_S3() {}

  void send_response();
};
class RGWPutObj_ObjStore_S3 : public RGWPutObj_ObjStore {
public:
  RGWPutObj_ObjStore_S3() {}
  ~RGWPutObj_ObjStore_S3() {}

  int get_params(){};
  void send_response();
};
class RGWInitMultipart_ObjStore_S3 : public RGWInitMultipart_ObjStore {
public:
  RGWInitMultipart_ObjStore_S3() {}
  ~RGWInitMultipart_ObjStore_S3() {}

  int get_params();
  void send_response();
};

#endif

