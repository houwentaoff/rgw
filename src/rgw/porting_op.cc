/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_op.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/11 14:31:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "porting_op.h"

int RGWOp::verify_op_mask()
{
  uint32_t required_mask = op_mask();

//  ldout(s->cct, 20) << "required_mask= " << required_mask << " user.op_mask=" << s->user.op_mask << dendl;

//  if ((s->user.op_mask & required_mask) != required_mask) {
//    return -EPERM;
//  }

  if (!s->system_request && (required_mask & RGW_OP_TYPE_MODIFY) /*  && !store->zone.is_master */)  {
    ldout(s->cct, 5) << "NOTICE: modify request to a non-master zone by a non-system user, permission denied"  << dendl;
    return -EPERM;
  }

  return 0;
}
#if 0
int RGWOp::init_quota()
{
  /* no quota enforcement for system requests */
  if (s->system_request)
    return 0;

  /* init quota related stuff */
  if (!(s->user.op_mask & RGW_OP_TYPE_MODIFY)) {
    return 0;
  }

  /* only interested in object related ops */
  if (s->object.empty()) {
    return 0;
  }

  RGWUserInfo owner_info;
  RGWUserInfo *uinfo;

  if (s->user.user_id == s->bucket_owner.get_id()) {
    uinfo = &s->user;
  } else {
    int r = rgw_get_user_info_by_uid(store, s->bucket_info.owner, owner_info);
    if (r < 0)
      return r;
    uinfo = &owner_info;
  }

  if (s->bucket_info.quota.enabled) {
    bucket_quota = s->bucket_info.quota;
  } else if (uinfo->bucket_quota.enabled) {
    bucket_quota = uinfo->bucket_quota;
  } else {
    bucket_quota = store->region_map.bucket_quota;
  }

  if (uinfo->user_quota.enabled) {
    user_quota = uinfo->user_quota;
  } else {
    user_quota = store->region_map.user_quota;
  }

  return 0;
}
#endif
int RGWOp::read_bucket_cors()
{
#if 0
  bufferlist bl;

  map<string, bufferlist>::iterator aiter = s->bucket_attrs.find(RGW_ATTR_CORS);
  if (aiter == s->bucket_attrs.end()) {
    ldout(s->cct, 20) << "no CORS configuration attr found" << dendl;
    cors_exist = false;
    return 0; /* no CORS configuration found */
  }

  cors_exist = true;

  bl = aiter->second;

  bufferlist::iterator iter = bl.begin();
  try {
    bucket_cors.decode(iter);
  } catch (buffer::error& err) {
    ldout(s->cct, 0) << "ERROR: could not decode policy, caught buffer::error" << dendl;
    return -EIO;
  }
  if (s->cct->_conf->subsys.should_gather(ceph_subsys_rgw, 15)) {
    RGWCORSConfiguration_S3 *s3cors = static_cast<RGWCORSConfiguration_S3 *>(&bucket_cors);
    ldout(s->cct, 15) << "Read RGWCORSConfiguration";
    s3cors->to_xml(*_dout);
    *_dout << dendl;
  }
#endif
  return 0;
}
RGWHandler::~RGWHandler()
{
}

int RGWHandler::init(RGWRados *_store, struct req_state *_s, RGWClientIO *cio)
{
  store = _store;
  s = _s;

  return 0;
}

int RGWHandler::do_read_permissions(RGWOp *op, bool only_bucket)
{
  int ret = 0;// rgw_build_policies(store, s, only_bucket, op->prefetch_data());
#if 0
  if (ret < 0) {
    ldout(s->cct, 10) << "read_permissions on " << s->bucket << ":" <<s->object << " only_bucket=" << only_bucket << " ret=" << ret << dendl;
    if (ret == -ENODATA)
      ret = -EACCES;
  }
#endif
  return ret;
}


RGWOp *RGWHandler::get_op(RGWRados *store)
{
  RGWOp *op;
  switch (s->op) {
   case OP_GET:
     op = op_get();
     break;
   case OP_PUT:
     op = op_put();
     break;
   case OP_DELETE:
     op = op_delete();
     break;
   case OP_HEAD:
     op = op_head();
     break;
   case OP_POST:
     op = op_post();
     break;
   case OP_COPY:
     op = op_copy();
     break;
   case OP_OPTIONS:
     op = op_options();
     break;
   default:
     return NULL;
  }

  if (op) {
    op->init(store, s, this);
  }
  return op;
}

void RGWHandler::put_op(RGWOp *op)
{
  delete op;
}

int RGWListBuckets::verify_permission()
{
  return 0;
}

void RGWListBuckets::execute()
{
  bool done;
  bool started = false;
  uint64_t total_count = 0;

  uint64_t max_buckets = G.rgw_list_buckets_max_chunk;//s->cct->_conf->rgw_list_buckets_max_chunk;

  ret = get_params();
  if (ret < 0) {
    goto send_end;
  }

  if (supports_account_metadata()) {
//    ret = rgw_get_user_attrs_by_uid(store, s->user.user_id, attrs);
    if (ret < 0) {
      goto send_end;
    }
  }

  do {
    RGWUserBuckets buckets;
    uint64_t read_count;
    if (limit >= 0) {
      read_count = min(limit - total_count, (uint64_t)max_buckets);
    } else {
      read_count = max_buckets;
    }

    ret = rgw_read_user_buckets(store, s->user.user_id, buckets,
                                marker, read_count, should_get_stats(), 0);
    RGWBucketEnt test;
    test.bucket.name = "bucket1";
    buckets.add(test);
    read_count=2;
    
    if (ret < 0) {
      /* hmm.. something wrong here.. the user was authenticated, so it
         should exist */
//      ldout(s->cct, 10) << "WARNING: failed on rgw_get_user_buckets uid=" << s->user.user_id << dendl;
      break;
    }
    map<string, RGWBucketEnt>& m = buckets.get_buckets();
    map<string, RGWBucketEnt>::iterator iter;
    for (iter = m.begin(); iter != m.end(); ++iter) {
      RGWBucketEnt& bucket = iter->second;
      buckets_size += bucket.size;
      buckets_size_rounded += bucket.size_rounded;
      buckets_objcount += bucket.count;

      marker = iter->first;
    }
    buckets_count += m.size();
    total_count += m.size();

    done = (m.size() < read_count || (limit >= 0 && total_count >= (uint64_t)limit));

    if (!started) {
      send_response_begin(buckets.count() > 0);
      started = true;
    }

    if (!m.empty()) {
      send_response_data(buckets);

      map<string, RGWBucketEnt>::reverse_iterator riter = m.rbegin();
      marker = riter->first;
    }
  } while (!done);

send_end:
  if (!started) {
    send_response_begin(false);
  }
  send_response_end();
}
void RGWListBucket::pre_exec()
{
//  rgw_bucket_object_pre_exec(s);
}
int RGWListBucket::parse_max_keys()
{
  if (!max_keys.empty()) {
    char *endptr;
    max = strtol(max_keys.c_str(), &endptr, 10);
    if (endptr) {
      while (*endptr && isspace(*endptr)) // ignore white space
        endptr++;
      if (*endptr) {
        return -EINVAL;
      }
    }
  } else {
    max = default_max;
  }

  return 0;
}
void RGWListBucket::execute()
{
  ret = get_params();
  if (ret < 0)
    return;

  if (need_container_stats()) {
    map<string, RGWBucketEnt> m;
//    m[s->bucket.name] = RGWBucketEnt();
//    m.begin()->second.bucket = s->bucket;
//    ret = store->update_containers_stats(m);
    if (ret > 0) {
      bucket = m.begin()->second;
    }
  }

//  RGWRados::Bucket target(store, s->bucket);
//  RGWRados::Bucket::List list_op(&target);
#if 0
  list_op.params.prefix = prefix;
  list_op.params.delim = delimiter;
  list_op.params.marker = marker;
  list_op.params.end_marker = end_marker;
  list_op.params.list_versions = list_versions;

  ret = list_op.list_objects(max, &objs, &common_prefixes, &is_truncated);
  if (ret >= 0 && !delimiter.empty()) {
    next_marker = list_op.get_next_marker();
  }
#endif
}

/**
 * Generate the CORS header response
 *
 * This is described in the CORS standard, section 6.2.
 */
bool RGWOp::generate_cors_headers(string& origin, string& method, string& headers, string& exp_headers, unsigned *max_age)
{
  /* CORS 6.2.1. */
  const char *orig = s->info.env->get("HTTP_ORIGIN");
  if (!orig) {
    return false;
  }

  /* Custom: */
  origin = orig;
  int ret = read_bucket_cors();
  if (ret < 0) {
    return false;
  }

  if (!cors_exist) {
    dout(2) << "No CORS configuration set yet for this bucket" << dendl;
    return false;
  }

  /* CORS 6.2.2. */
//  RGWCORSRule *rule = bucket_cors.host_name_rule(orig);
//  if (!rule)
//    return false;

  /* CORS 6.2.3. */
  const char *req_meth = s->info.env->get("HTTP_ACCESS_CONTROL_REQUEST_METHOD");
  if (!req_meth) {
    req_meth = s->info.method;
  }

  if (req_meth) {
    method = req_meth;
    /* CORS 6.2.5. */
//    if (!validate_cors_rule_method(rule, req_meth)) {
//     return false;
//    }
  }

  /* CORS 6.2.4. */
  const char *req_hdrs = s->info.env->get("HTTP_ACCESS_CONTROL_REQUEST_HEADERS");

  /* CORS 6.2.6. */
//  get_cors_response_headers(rule, req_hdrs, headers, exp_headers, max_age);

  return true;
}



