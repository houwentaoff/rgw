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
#include "porting_common.h"
#include "porting_rest.h"
#include "porting_rados.h"
#include "global/global.h"

static int parse_range(const char *range, off_t& ofs, off_t& end, bool *partial_content)
{
  int r = -ERANGE;
  string s(range);
  string ofs_str;
  string end_str;

  *partial_content = false;

  int pos = s.find("bytes=");
  if (pos < 0) {
    pos = 0;
    while (isspace(s[pos]))
      pos++;
    int end = pos;
    while (isalpha(s[end]))
      end++;
    if (strncasecmp(s.c_str(), "bytes", end - pos) != 0)
      return 0;
    while (isspace(s[end]))
      end++;
    if (s[end] != '=')
      return 0;
    s = s.substr(end + 1);
  } else {
    s = s.substr(pos + 6); /* size of("bytes=")  */
  }
  pos = s.find('-');
  if (pos < 0)
    goto done;

  *partial_content = true;

  ofs_str = s.substr(0, pos);
  end_str = s.substr(pos + 1);
  if (end_str.length()) {
    end = atoll(end_str.c_str());
    if (end < 0)
      goto done;
  }

  if (ofs_str.length()) {
    ofs = atoll(ofs_str.c_str());
  } else { // RFC2616 suffix-byte-range-spec
    ofs = -end;
    end = -1;
  }

  if (end >= 0 && end < ofs)
    goto done;

  r = 0;
done:
  return r;
}

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

/**
 * Get the AccessControlPolicy for a bucket or object off of disk.
 * s: The req_state to draw information from.
 * only_bucket: If true, reads the bucket ACL rather than the object ACL.
 * Returns: 0 on success, -ERR# otherwise.
 */
static int rgw_build_policies(RGWRados *store, struct req_state *s, bool only_bucket, bool prefetch_data)
{
  int ret = 0;
  rgw_obj_key obj;
  RGWUserInfo bucket_owner_info;
  RGWObjectCtx& obj_ctx = *static_cast<RGWObjectCtx *>(s->obj_ctx);

  string bi = s->info.args.get(RGW_SYS_PARAM_PREFIX "bucket-instance");
  if (!bi.empty()) {
    int shard_id;
    ret = rgw_bucket_parse_bucket_instance(bi, &s->bucket_instance_id, &shard_id);
    if (ret < 0) {
      return ret;
    }
  }

  if(s->dialect.compare("s3") == 0) {
//    s->bucket_acl = new RGWAccessControlPolicy_S3(s->cct);
  } else if(s->dialect.compare("swift")  == 0) {
    //s->bucket_acl = new RGWAccessControlPolicy_SWIFT(s->cct);
  } else {
    //s->bucket_acl = new RGWAccessControlPolicy(s->cct);
  }

  if (s->copy_source) { /* check if copy source is within the current domain */
    const char *src = s->copy_source;
    if (*src == '/')
      ++src;
    string copy_source_str(src);

    int pos = copy_source_str.find('/');
    if (pos > 0)
      copy_source_str = copy_source_str.substr(0, pos);

    RGWBucketInfo source_info;

    ret = store->get_bucket_info(obj_ctx, copy_source_str, source_info, NULL);
    if (ret == 0) {//bucket is exist ????
      string& region = source_info.region;
//      s->local_source = store->region.equals(region);
    }
  }

  if (!s->bucket_name_str.empty()) {
    s->bucket_exists = true;
    if (s->bucket_instance_id.empty()) {
      ret = store->get_bucket_info(obj_ctx, s->bucket_name_str, s->bucket_info, NULL, &s->bucket_attrs);
    } else {
//      ret = store->get_bucket_instance_info(obj_ctx, s->bucket_instance_id, s->bucket_info, NULL, &s->bucket_attrs);
    }
    if (ret < 0) {//is not exist ....
      if (ret != -ENOENT) {
        ldout(s->cct, 0) << "NOTICE: couldn't get bucket from bucket_name (name=" << s->bucket_name_str << ")" << dendl;
        return ret;
      }
      s->bucket_exists = false;
    }
    s->bucket = s->bucket_info.bucket;

    if (s->bucket_exists) {
      rgw_obj_key no_obj;
//      ret = read_policy(store, s, s->bucket_info, s->bucket_attrs, s->bucket_acl, s->bucket, no_obj);
    } else {
//      s->bucket_acl->create_default(s->user.user_id, s->user.display_name);
      ret = -ERR_NO_SUCH_BUCKET;
    }

//    s->bucket_owner = s->bucket_acl->get_owner();

    string& region = s->bucket_info.region;
//    map<string, RGWRegion>::iterator dest_region = store->region_map.regions.find(region);
//    if (dest_region != store->region_map.regions.end() && !dest_region->second.endpoints.empty()) {
//      s->region_endpoint = dest_region->second.endpoints.front();
//    }
#if 0    //redirect to other region
    if (s->bucket_exists /*&& !store->region.equals(region)*/) {
        //s->local_source = true;//add by sean or it return ERR_PERMANENT_REDIRECT
//      ldout(s->cct, 0) << "NOTICE: request for data in a different region (" << region << " != " << store->region.name << ")" << dendl;
      /* we now need to make sure that the operation actually requires copy source, that is
       * it's a copy operation
       */
      if (/*store->region.is_master && */s->op == OP_DELETE && s->system_request) {
        /*If the operation is delete and if this is the master, don't redirect*/
      } else if (!s->local_source ||
          (s->op != OP_PUT && s->op != OP_COPY) ||
          s->object.empty()) {
        return -ERR_PERMANENT_REDIRECT;
      }
    }
#endif
  }

  /* we're passed only_bucket = true when we specifically need the bucket's
     acls, that happens on write operations */
  if (!only_bucket && !s->object.empty()) {
    if (!s->bucket_exists) {
      return -ERR_NO_SUCH_BUCKET;
    }
//    s->object_acl = new RGWAccessControlPolicy(s->cct);

    rgw_obj obj(s->bucket, s->object);
    store->set_atomic(s->obj_ctx, obj);
    if (prefetch_data) {
      store->set_prefetch_data(s->obj_ctx, obj);
    }
//    ret = read_policy(store, s, s->bucket_info, s->bucket_attrs, s->object_acl, s->bucket, s->object);
  }

  return ret;
}

int RGWHandler::do_read_permissions(RGWOp *op, bool only_bucket)
{
  int ret = rgw_build_policies(store, s, only_bucket, op->prefetch_data());
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
//    RGWBucketEnt test;
//    test.bucket.name = "bucket1";
//    buckets.add(test);
//    read_count=2;
    
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

int RGWListBucket::verify_permission()
{
  if (!verify_bucket_permission(s, RGW_PERM_READ))
    return -EACCES;

  return 0;
}

static void rgw_bucket_object_pre_exec(struct req_state *s)
{
  if (s->expect_cont)
    dump_continue(s);

  dump_bucket_from_state(s);
}

void RGWListBucket::pre_exec()
{
  rgw_bucket_object_pre_exec(s);
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

  if (need_container_stats()) {//just swift need
    map<string, RGWBucketEnt> m;
    m[s->bucket.name] = RGWBucketEnt();
    m.begin()->second.bucket = s->bucket;
    ret = 0;//store->update_containers_stats(m);
    if (ret > 0) {
      bucket = m.begin()->second;
    }
  }
#if 0  // is ok
  RGWObjEnt ent;
  rgw_obj_key aaa("helloo.txt");
  ent.key =  aaa;
  ent.ns = "ns"; 
  ent.owner = "hwttt";
  ent.size = 10000;
  ent.etag = "etag";
  ent.tag = "tagg";
  ent.owner_display_name = "htw display";
  ent.content_type = ".jpg";
  objs.push_back(ent);
  ent.owner = "aaaaaas";
  ent.size = 100;
  objs.push_back(ent);
#endif
  RGWRados::Bucket target(store, s->bucket);
  RGWRados::Bucket::List list_op(&target);
#if 1
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

class RGWGetObj_CB : public RGWGetDataCB
{
  RGWGetObj *op;
public:
  RGWGetObj_CB(RGWGetObj *_op) : op(_op) {}
  virtual ~RGWGetObj_CB() {}

  int handle_data(bufferlist& bl, off_t bl_ofs, off_t bl_len) {
//    return op->get_data_cb(bl, bl_ofs, bl_len);
  }
};

int RGWGetObj::init_common()
{
  if (range_str) {
    /* range parsed error when prefetch*/
    if (!range_parsed) {
      int r = parse_range(range_str, ofs, end, &partial_content);
      if (r < 0)
        return r;
    }
  }
  if (if_mod) {
    if (parse_time(if_mod, &mod_time) < 0)
      return -EINVAL;
    mod_ptr = &mod_time;
  }

  if (if_unmod) {
    if (parse_time(if_unmod, &unmod_time) < 0)
      return -EINVAL;
    unmod_ptr = &unmod_time;
  }

  return 0;
}

bool RGWGetObj::prefetch_data()
{
  /* HEAD request, stop prefetch*/
  if (!get_data) {
    return false;
  }

  bool prefetch_first_chunk = true;
  range_str = s->info.env->get("HTTP_RANGE");

  if(range_str) {
    int r = parse_range(range_str, ofs, end, &partial_content);
    /* error on parsing the range, stop prefetch and will fail in execte() */
    if (r < 0) {
      range_parsed = false;
      return false;
    } else {
      range_parsed = true;
    }
    /* range get goes to shadown objects, stop prefetch */
//    if (ofs >= s->cct->_conf->rgw_max_chunk_size) {
//      prefetch_first_chunk = false;
//    }
  }

  return get_data && prefetch_first_chunk;
}
void RGWGetObj::pre_exec()
{
  rgw_bucket_object_pre_exec(s);
}

void RGWGetObj::execute()
{
  utime_t start_time = s->time;
  bufferlist bl;
//  gc_invalidate_time = ceph_clock_now(s->cct);
  gc_invalidate_time += 10;//(s->cct->_conf->rgw_gc_obj_min_wait / 2);

  RGWGetObj_CB cb(this);

  map<string, bufferlist>::iterator attr_iter;

//  perfcounter->inc(l_rgw_get);
  int64_t new_ofs, new_end;

  RGWRados::Object op_target(store, s->bucket_info, *static_cast<RGWObjectCtx *>(s->obj_ctx), obj);
  RGWRados::Object::Read read_op(&op_target);

  ret = get_params();
  if (ret < 0)
    goto done_err;

  ret = init_common();
  if (ret < 0)
    goto done_err;

  new_ofs = ofs;
  new_end = end;

  read_op.conds.mod_ptr = mod_ptr;
  read_op.conds.unmod_ptr = unmod_ptr;
  read_op.conds.if_match = if_match;
  read_op.conds.if_nomatch = if_nomatch;
  read_op.params.attrs = &attrs;
  read_op.params.lastmod = &lastmod;
  read_op.params.read_size = &total_len;
  read_op.params.obj_size = &s->obj_size;
  read_op.params.perr = &s->err;

//  ret = read_op.prepare(&new_ofs, &new_end);
  if (ret < 0)
    goto done_err;

  attr_iter = attrs.find(RGW_ATTR_USER_MANIFEST);
  if (attr_iter != attrs.end() && !skip_manifest) {
//    ret = handle_user_manifest(attr_iter->second.c_str());
    if (ret < 0) {
      ldout(s->cct, 0) << "ERROR: failed to handle user manifest ret=" << ret << dendl;
    }
    return;
  }

  /* Check whether the object has expired. Swift API documentation
   * stands that we should return 404 Not Found in such case. */
//  if (need_object_expiration() && object_is_expired(attrs)) {
//    ret = -ENOENT;
//    goto done_err;
//  }

  ofs = new_ofs;
  end = new_end;

  start = ofs;

  if (!get_data || ofs > end)
    goto done_err;

//  perfcounter->inc(l_rgw_get_b, end - ofs);

//  ret = read_op.iterate(ofs, end, &cb);

//  perfcounter->tinc(l_rgw_get_lat,
//                   (ceph_clock_now(s->cct) - start_time));
  if (ret < 0) {
    goto done_err;
  }

  send_response_data(bl, 0, 0);
  return;

done_err:
  send_response_data_error();
}



