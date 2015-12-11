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
//  RGWQuotaInfo bucket_quota;
//  RGWQuotaInfo user_quota;

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
#endif
