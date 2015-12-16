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

#include "porting_rest_s3.h"

static bool looks_like_ip_address(const char *bucket)
{
  int num_periods = 0;
  bool expect_period = false;
  for (const char *b = bucket; *b; ++b) {
    if (*b == '.') {
      if (!expect_period)
	return false;
      ++num_periods;
      if (num_periods > 3)
	return false;
      expect_period = false;
    }
    else if (isdigit(*b)) {
      expect_period = true;
    }
    else {
      return false;
    }
  }
  return (num_periods == 3);
}

int RGWHandler_Auth_S3::init(RGWRados *store, struct req_state *state, RGWClientIO *cio)
{
  int ret = RGWHandler_ObjStore_S3::init_from_header(state, RGW_FORMAT_JSON, true);
  if (ret < 0)
    return ret;

  return RGWHandler_ObjStore::init(store, state, cio);
}
/*
 * verify that a signed request comes from the keyholder
 * by checking the signature against our locally-computed version
 */
int RGW_Auth_S3::authorize(RGWRados *store, struct req_state *s)
{
    return 0;
}
int RGWHandler_ObjStore_S3::validate_bucket_name(const string& bucket, bool relaxed_names)
{
  int ret = RGWHandler_ObjStore::validate_bucket_name(bucket);
  if (ret < 0)
    return ret;

  if (bucket.size() == 0)
    return 0;

  // bucket names must start with a number, letter, or underscore
  if (!(isalpha(bucket[0]) || isdigit(bucket[0]))) {
    if (!relaxed_names)
      return -ERR_INVALID_BUCKET_NAME;
    else if (!(bucket[0] == '_' || bucket[0] == '.' || bucket[0] == '-'))
      return -ERR_INVALID_BUCKET_NAME;
  } 

  for (const char *s = bucket.c_str(); *s; ++s) {
    char c = *s;
    if (isdigit(c) || (c == '.'))
      continue;
    if (isalpha(c))
      continue;
    if ((c == '-') || (c == '_'))
      continue;
    // Invalid character
    return -ERR_INVALID_BUCKET_NAME;
  }

  if (looks_like_ip_address(bucket.c_str()))
    return -ERR_INVALID_BUCKET_NAME;

  return 0;
}

int RGWHandler_ObjStore_S3::init(RGWRados *store, struct req_state *s, RGWClientIO *cio)
{
  dout(10) << "s->object=" << (!s->object.empty() ? s->object : rgw_obj_key("<NULL>")) << " s->bucket=" << (!s->bucket_name_str.empty() ? s->bucket_name_str : "<NULL>") << dendl;

  bool relaxed_names = false;//= s->cct->_conf->rgw_relaxed_s3_bucket_names;
  int ret = validate_bucket_name(s->bucket_name_str, relaxed_names);
  if (ret)
    return ret;
  ret = validate_object_name(s->object.name);
  if (ret)
    return ret;

  const char *cacl = s->info.env->get("HTTP_X_AMZ_ACL");
  if (cacl)
    s->canned_acl = cacl;

  s->has_acl_header = s->info.env->exists_prefix("HTTP_X_AMZ_GRANT");

  s->copy_source = s->info.env->get("HTTP_X_AMZ_COPY_SOURCE");
  if (s->copy_source) {
    ret = 0;//= RGWCopyObj::parse_copy_location(s->copy_source, s->src_bucket_name, s->src_object);
    if (!ret) {
      ldout(s->cct, 0) << "failed to parse copy location" << dendl;
      return -EINVAL;
    }
  }

  s->dialect = "s3";

  return RGWHandler_ObjStore::init(store, s, cio);
}

int RGWHandler_ObjStore_S3::init_from_header(struct req_state *s, int default_formatter, bool configurable_format)
{
    string req;
    string first;

    const char *req_name = s->relative_uri.c_str();
    const char *p;

    if (*req_name == '?') {
        p = req_name;
    } else {
        p = s->info.request_params.c_str();
    }

    s->info.args.set(p);
    s->info.args.parse();
    /* must be called after the args parsing */
    int ret = allocate_formatter(s, default_formatter, configurable_format);
    if (ret < 0)
        return ret;

    if (*req_name != '/')
        return 0;

    req_name++;

    if (!*req_name)
        return 0;

    req = req_name;
    int pos = req.find('/');
    if (pos >= 0) {
        first = req.substr(0, pos);
    } else {
        first = req;
    }

    if (s->bucket_name_str.empty()) {
        s->bucket_name_str = first;

        if (pos >= 0) {
            string encoded_obj_str = req.substr(pos+1);
            s->object = rgw_obj_key(encoded_obj_str, s->info.args.get("versionId"));
        }
    } else {
        s->object = rgw_obj_key(req_name, s->info.args.get("versionId"));
    }
    return 0;
}

RGWHandler *RGWRESTMgr_S3::get_handler(struct req_state *s)
{
  int ret = RGWHandler_ObjStore_S3::init_from_header(s, RGW_FORMAT_XML, false);
  if (ret < 0)
    return NULL;

  if (s->bucket_name_str.empty())
    return new RGWHandler_ObjStore_Service_S3;

  if (s->object.empty())
    return new RGWHandler_ObjStore_Bucket_S3;

  return new RGWHandler_ObjStore_Obj_S3;
}
RGWOp *RGWHandler_ObjStore_Service_S3::op_get()
{
  return NULL;//new RGWListBuckets_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Service_S3::op_head()
{
  return NULL;//new RGWListBuckets_ObjStore_S3;
}


RGWOp *RGWHandler_ObjStore_Obj_S3::get_obj_op(bool get_data)
{
  if (is_acl_op()) {
    return NULL;//new RGWGetACLs_ObjStore_S3;
  }
//  RGWGetObj_ObjStore_S3 *get_obj_op = NULL;//new RGWGetObj_ObjStore_S3;
//  get_obj_op->set_get_data(get_data);
  return NULL;//get_obj_op;
}
RGWOp *RGWHandler_ObjStore_Obj_S3::op_get()
{
  if (is_acl_op()) {
    return NULL;//new RGWGetACLs_ObjStore_S3;
  } else if (s->info.args.exists("uploadId")) {
    return NULL;// new RGWListMultipart_ObjStore_S3;
  }
  return get_obj_op(true);
}

RGWOp *RGWHandler_ObjStore_Obj_S3::op_head()
{
  if (is_acl_op()) {
    return NULL;//new RGWGetACLs_ObjStore_S3;
  } else if (s->info.args.exists("uploadId")) {
    return NULL;//new RGWListMultipart_ObjStore_S3;
  }
  return get_obj_op(false);
}

RGWOp *RGWHandler_ObjStore_Obj_S3::op_put()
{
  if (is_acl_op()) {
    return NULL;// new RGWPutACLs_ObjStore_S3;
  }
  if (!s->copy_source)
    return NULL;//new RGWPutObj_ObjStore_S3;
  else
    return NULL;//new RGWCopyObj_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Obj_S3::op_delete()
{
  string upload_id = s->info.args.get("uploadId");

  if (upload_id.empty())
    return NULL;//new RGWDeleteObj_ObjStore_S3;
  else
    return NULL;//new RGWAbortMultipart_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Obj_S3::op_post()
{
  if (s->info.args.exists("uploadId"))
    return NULL;//new RGWCompleteMultipart_ObjStore_S3;

  if (s->info.args.exists("uploads"))
    return NULL;//new RGWInitMultipart_ObjStore_S3;

  return NULL;
}

RGWOp *RGWHandler_ObjStore_Obj_S3::op_options()
{
//  return new RGWOptionsCORS_ObjStore_S3;
    return NULL;
}
RGWOp *RGWHandler_ObjStore_Bucket_S3::get_obj_op(bool get_data)
{
  if (get_data)
    return NULL;//new RGWListBucket_ObjStore_S3;
  else
    return NULL;//new RGWStatBucket_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_get()
{
  if (s->info.args.sub_resource_exists("logging"))
    return NULL;//new RGWGetBucketLogging_ObjStore_S3;

  if (s->info.args.sub_resource_exists("location"))
    return NULL;//new RGWGetBucketLocation_ObjStore_S3;

  if (s->info.args.sub_resource_exists("versioning"))
    return NULL;//new RGWGetBucketVersioning_ObjStore_S3;

  if (is_acl_op()) {
    return NULL;//new RGWGetACLs_ObjStore_S3;
  } else if (is_cors_op()) {
    return NULL;//new RGWGetCORS_ObjStore_S3;
  } else if (is_request_payment_op()) {
    return NULL;//new RGWGetRequestPayment_ObjStore_S3;
  } else if (s->info.args.exists("uploads")) {
    return NULL;//new RGWListBucketMultiparts_ObjStore_S3;
  }
  return get_obj_op(true);
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_head()
{
  if (is_acl_op()) {
    return NULL;//new RGWGetACLs_ObjStore_S3;
  } else if (s->info.args.exists("uploads")) {
    return NULL;//new RGWListBucketMultiparts_ObjStore_S3;
  }
  return get_obj_op(false);
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_put()
{
  if (s->info.args.sub_resource_exists("logging"))
    return NULL;
  if (s->info.args.sub_resource_exists("versioning"))
    return NULL;//new RGWSetBucketVersioning_ObjStore_S3;
  if (is_acl_op()) {
    return NULL;//new RGWPutACLs_ObjStore_S3;
  } else if (is_cors_op()) {
    return NULL;//new RGWPutCORS_ObjStore_S3;
  } else if (is_request_payment_op()) {
    return NULL;//new RGWSetRequestPayment_ObjStore_S3;
  }
  return NULL;//new RGWCreateBucket_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_delete()
{
  if (is_cors_op()) {
    return NULL;//new RGWDeleteCORS_ObjStore_S3;
  }
  return NULL;//new RGWDeleteBucket_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_post()
{
  if ( s->info.request_params == "delete" ) {
    return NULL;//new RGWDeleteMultiObj_ObjStore_S3;
  }

  return NULL;//new RGWPostObj_ObjStore_S3;
}

RGWOp *RGWHandler_ObjStore_Bucket_S3::op_options()
{
  return NULL;//new RGWOptionsCORS_ObjStore_S3;
}



