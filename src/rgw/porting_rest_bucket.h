/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest_bucket.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/14 16:51:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef  __PORTING_REST_BUCKET_H__
#define  __PORTING_REST_BUCKET_H__
#include "porting_rest_s3.h"
#include "porting_rest.h"
class RGWHandler_Bucket : public RGWHandler_Auth_S3 {
protected:
  RGWOp *op_get();
  RGWOp *op_put();
  RGWOp *op_post();
  RGWOp *op_delete();
public:
  RGWHandler_Bucket() {}
  virtual ~RGWHandler_Bucket() {}

  int read_permissions(RGWOp*) {
    return 0;
  }
};
class RGWRESTMgr_Bucket : public RGWRESTMgr {
public:
  RGWRESTMgr_Bucket() {}
  virtual ~RGWRESTMgr_Bucket() {}

  RGWHandler *get_handler(struct req_state *s) {
    return new RGWHandler_Bucket;
  }
};
#endif
