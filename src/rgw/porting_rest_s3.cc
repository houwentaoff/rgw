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
#include "global/global.h"

struct response_attr_param {
  const char *param;
  const char *http_attr;
};

static struct response_attr_param resp_attr_params[] = {
  {"response-content-type", "Content-Type"},
  {"response-content-language", "Content-Language"},
  {"response-expires", "Expires"},
  {"response-cache-control", "Cache-Control"},
  {"response-content-disposition", "Content-Disposition"},
  {"response-content-encoding", "Content-Encoding"},
  {NULL, NULL},
};

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

  bool relaxed_names = G.rgw_relaxed_s3_bucket_names;//= s->cct->_conf->rgw_relaxed_s3_bucket_names;
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
  return new RGWListBuckets_ObjStore_S3;
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
  RGWGetObj_ObjStore_S3 *get_obj_op = new RGWGetObj_ObjStore_S3;
  get_obj_op->set_get_data(get_data);
  return get_obj_op;
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
    return new RGWPutObj_ObjStore_S3;
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
    return new RGWListBucket_ObjStore_S3;
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
  return new RGWCreateBucket_ObjStore_S3;
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
void RGWListBuckets_ObjStore_S3::send_response_begin(bool has_buckets)
{
  //if (ret)
  //  set_req_state_err(s, ret);
  dump_errno(s);
  dump_start(s);
  end_header(s, NULL, "application/xml");

  if (!ret) {
    list_all_buckets_start(s);
//    dump_owner(s, s->user.user_id, s->user.display_name);
    s->formatter->open_array_section("Buckets");
    sent_data = true;
  }
}
void dump_bucket(struct req_state *s, RGWBucketEnt& obj)
{
  s->formatter->open_object_section("Bucket");
  s->formatter->dump_string("Name", obj.bucket.name);
  dump_time(s, "CreationDate", &obj.creation_time);
  s->formatter->close_section();
}

void RGWListBuckets_ObjStore_S3::send_response_data(RGWUserBuckets& buckets)
{
  if (!sent_data)
    return;

  map<string, RGWBucketEnt>& m = buckets.get_buckets();
  map<string, RGWBucketEnt>::iterator iter;

  for (iter = m.begin(); iter != m.end(); ++iter) {
    RGWBucketEnt obj = iter->second;
    dump_bucket(s, obj);
  }
  rgw_flush_formatter(s, s->formatter);
}

void RGWListBuckets_ObjStore_S3::send_response_end()
{
  if (sent_data) {
    s->formatter->close_section();
    list_all_buckets_end(s);
    rgw_flush_formatter_and_reset(s, s->formatter);
  }
}

void list_all_buckets_start(struct req_state *s)
{
  s->formatter->open_array_section_in_ns("ListAllMyBucketsResult",
			      "http://s3.amazonaws.com/doc/2006-03-01/");
}

void list_all_buckets_end(struct req_state *s)
{
  s->formatter->close_section();
}

int RGWListBucket_ObjStore_S3::get_params()
{
  list_versions = s->info.args.exists("versions");
  prefix = s->info.args.get("prefix");
  if (!list_versions) {
    marker = s->info.args.get("marker");
  } else {
    marker.name = s->info.args.get("key-marker");
    marker.instance = s->info.args.get("version-id-marker");
  }
  max_keys = s->info.args.get("max-keys");
  ret = parse_max_keys();
  if (ret < 0) {
    return ret;
  }
  delimiter = s->info.args.get("delimiter");
  encoding_type = s->info.args.get("encoding-type");
  return 0;
}

void RGWListBucket_ObjStore_S3::send_versioned_response()
{
  s->formatter->open_object_section_in_ns("ListVersionsResult",
					  "http://s3.amazonaws.com/doc/2006-03-01/");
  s->formatter->dump_string("Name", s->bucket_name_str);
  s->formatter->dump_string("Prefix", prefix);
  s->formatter->dump_string("KeyMarker", marker.name);
  if (is_truncated && !next_marker.empty())
    s->formatter->dump_string("NextKeyMarker", next_marker.name);
  s->formatter->dump_int("MaxKeys", max);
  if (!delimiter.empty())
    s->formatter->dump_string("Delimiter", delimiter);

  s->formatter->dump_string("IsTruncated", (max && is_truncated ? "true" : "false"));

  bool encode_key = false;
  if (strcasecmp(encoding_type.c_str(), "url") == 0)
    encode_key = true;

  if (ret >= 0) {
    vector<RGWObjEnt>::iterator iter;
    for (iter = objs.begin(); iter != objs.end(); ++iter) {
      time_t mtime = iter->mtime.sec();
      const char *section_name = (iter->is_delete_marker() ? "DeleteMarker" : "Version");
      s->formatter->open_array_section(section_name);
      if (encode_key) {
        string key_name;
        url_encode(iter->key.name, key_name);
        s->formatter->dump_string("Key", key_name);
      } else {
        s->formatter->dump_string("Key", iter->key.name);
      }
      string version_id = iter->key.instance;
      if (version_id.empty()) {
        version_id = "null";
      }
      if (s->system_request && iter->versioned_epoch > 0) {
        s->formatter->dump_int("VersionedEpoch", iter->versioned_epoch);
      }
      s->formatter->dump_string("VersionId", version_id);
      s->formatter->dump_bool("IsLatest", iter->is_current());
      dump_time(s, "LastModified", &mtime);
      if (!iter->is_delete_marker()) {
        s->formatter->dump_format("ETag", "\"%s\"", iter->etag.c_str());
        s->formatter->dump_int("Size", iter->size);
        s->formatter->dump_string("StorageClass", "STANDARD");
      }
      dump_owner(s, iter->owner, iter->owner_display_name);
      s->formatter->close_section();
    }
    if (!common_prefixes.empty()) {
      map<string, bool>::iterator pref_iter;
      for (pref_iter = common_prefixes.begin(); pref_iter != common_prefixes.end(); ++pref_iter) {
        s->formatter->open_array_section("CommonPrefixes");
        s->formatter->dump_string("Prefix", pref_iter->first);
        s->formatter->close_section();
      }
    }
  }
  s->formatter->close_section();
  rgw_flush_formatter_and_reset(s, s->formatter);
}

void RGWListBucket_ObjStore_S3::send_response()
{
  if (ret < 0)
    set_req_state_err(s, ret);
  dump_errno(s);

  end_header(s, this, "application/xml");
  dump_start(s);
  if (ret < 0)
    return;

  if (list_versions) {
    send_versioned_response();
    return;
  }

  s->formatter->open_object_section_in_ns("ListBucketResult",
					  "http://s3.amazonaws.com/doc/2006-03-01/");
  s->formatter->dump_string("Name", s->bucket_name_str);
  s->formatter->dump_string("Prefix", prefix);
  s->formatter->dump_string("Marker", marker.name);
  if (is_truncated && !next_marker.empty())
    s->formatter->dump_string("NextMarker", next_marker.name);
  s->formatter->dump_int("MaxKeys", max);
  if (!delimiter.empty())
    s->formatter->dump_string("Delimiter", delimiter);

  s->formatter->dump_string("IsTruncated", (max && is_truncated ? "true" : "false"));

  bool encode_key = false;
  if (strcasecmp(encoding_type.c_str(), "url") == 0)
    encode_key = true;

  if (ret >= 0) {
    vector<RGWObjEnt>::iterator iter;
    for (iter = objs.begin(); iter != objs.end(); ++iter) {
      s->formatter->open_array_section("Contents");
      if (encode_key) {
        string key_name;
        url_encode(iter->key.name, key_name);
        s->formatter->dump_string("Key", key_name);
      } else {
        s->formatter->dump_string("Key", iter->key.name);
      }
      time_t mtime = iter->mtime.sec();
      dump_time(s, "LastModified", &mtime);
      s->formatter->dump_format("ETag", "\"%s\"", iter->etag.c_str());
      s->formatter->dump_int("Size", iter->size);
      s->formatter->dump_string("StorageClass", "STANDARD");
      dump_owner(s, iter->owner, iter->owner_display_name);
      s->formatter->close_section();
    }
    if (!common_prefixes.empty()) {
      map<string, bool>::iterator pref_iter;
      for (pref_iter = common_prefixes.begin(); pref_iter != common_prefixes.end(); ++pref_iter) {
        s->formatter->open_array_section("CommonPrefixes");
        s->formatter->dump_string("Prefix", pref_iter->first);
        s->formatter->close_section();
      }
    }
  }
  s->formatter->close_section();
  rgw_flush_formatter_and_reset(s, s->formatter);
}

int RGWGetObj_ObjStore_S3::send_response_data_error()
{
  bufferlist bl;
  return send_response_data(bl, 0 , 0);
}

int RGWGetObj_ObjStore_S3::send_response_data(bufferlist& bl, off_t bl_ofs, off_t bl_len)
{
  const char *content_type = NULL;
  string content_type_str;
  map<string, string> response_attrs;
  map<string, string>::iterator riter;
  bufferlist metadata_bl;

  if (ret)
    goto done;

  if (sent_header)
    goto send_data;

  if (range_str)
    dump_range(s, start, end, s->obj_size);

  if (s->system_request &&
      s->info.args.exists(RGW_SYS_PARAM_PREFIX "prepend-metadata")) {

    /* JSON encode object metadata */
    JSONFormatter jf;
    jf.open_object_section("obj_metadata");
    encode_json("attrs", attrs, &jf);
    encode_json("mtime", lastmod, &jf);
    jf.close_section();
    stringstream ss;
    jf.flush(ss);
    metadata_bl.append(ss.str());
    s->cio->print("Rgwx-Embedded-Metadata-Len: %lld\r\n", (long long)metadata_bl.length());
    total_len += metadata_bl.length();
  }

  if (s->system_request && lastmod) {
    /* we end up dumping mtime in two different methods, a bit redundant */
    dump_epoch_header(s, "Rgwx-Mtime", lastmod);
  }

  dump_content_length(s, total_len);
  dump_last_modified(s, lastmod);

  if (!ret) {
    map<string, bufferlist>::iterator iter = attrs.find(RGW_ATTR_ETAG);
    if (iter != attrs.end()) {
      bufferlist& bl = iter->second;
      if (bl.length()) {
        char *etag = bl.c_str();
        dump_etag(s, etag);
      }
    }

    for (struct response_attr_param *p = resp_attr_params; p->param; p++) {
      bool exists;
      string val = s->info.args.get(p->param, &exists);
      if (exists) {
	if (strcmp(p->param, "response-content-type") != 0) {
	  response_attrs[p->http_attr] = val;
	} else {
	  content_type_str = val;
	  content_type = content_type_str.c_str();
	}
      }
    }

    for (iter = attrs.begin(); iter != attrs.end(); ++iter) {
      const char *name = iter->first.c_str();

      map<string, string>::iterator aiter = rgw_to_http_attrs.find(name);
      if (aiter != rgw_to_http_attrs.end()) {
        if (response_attrs.count(aiter->second) == 0) {
          /* Was not already overridden by a response param. */
          response_attrs[aiter->second] = iter->second.c_str();
        }
      } else if (iter->first.compare(RGW_ATTR_CONTENT_TYPE) == 0) {
        /* Special handling for content_type. */
        if (!content_type) {
          content_type = iter->second.c_str();
        }
      } else if (strncmp(name, RGW_ATTR_META_PREFIX, sizeof(RGW_ATTR_META_PREFIX)-1) == 0) {
        /* User custom metadata. */
        name += sizeof(RGW_ATTR_PREFIX) - 1;
        s->cio->print("%s: %s\r\n", name, iter->second.c_str());
      }
    }
  }

done:
  set_req_state_err(s, (partial_content && !ret) ? STATUS_PARTIAL_CONTENT : ret);

  dump_errno(s);

  for (riter = response_attrs.begin(); riter != response_attrs.end(); ++riter) {
    s->cio->print("%s: %s\r\n", riter->first.c_str(), riter->second.c_str());
  }

  if (!content_type)
    content_type = "binary/octet-stream";

  end_header(s, this, content_type);

  if (metadata_bl.length()) {
    s->cio->write(metadata_bl.c_str(), metadata_bl.length());
  }
  sent_header = true;

send_data:
  if (get_data && !ret) {
    int r = s->cio->write(bl.c_str() + bl_ofs, bl_len);
    if (r < 0)
      return r;
  }

  return 0;
}
int RGWCreateBucket_ObjStore_S3::get_params()
{
  return 0;    
}
void RGWCreateBucket_ObjStore_S3::send_response()
{
  if (ret == -ERR_BUCKET_EXISTS)
    ret = 0;
  if (ret)
    set_req_state_err(s, ret);
  dump_errno(s);
  end_header(s);

  if (ret < 0)
    return;

  if (s->system_request) {
    JSONFormatter f; /* use json formatter for system requests output */

    f.open_object_section("info");
    encode_json("entry_point_object_ver", ep_objv, &f);
    encode_json("object_ver", info.objv_tracker.read_version, &f);
    encode_json("bucket_info", info, &f);
    f.close_section();
    rgw_flush_formatter_and_reset(s, &f);
  }
}
void RGWDeleteBucket_ObjStore_S3::send_response()
{
  int r = ret;
  if (!r)
    r = STATUS_NO_CONTENT;

  set_req_state_err(s, r);
  dump_errno(s);
  end_header(s, this);

  if (s->system_request) {
    JSONFormatter f; /* use json formatter for system requests output */

    f.open_object_section("info");
    encode_json("object_ver", objv_tracker.read_version, &f);
    f.close_section();
    rgw_flush_formatter_and_reset(s, &f);
  }
}
static int get_success_retcode(int code)
{
  switch (code) {
    case 201:
      return STATUS_CREATED;
    case 204:
      return STATUS_NO_CONTENT;
  }
  return 0;
}
void RGWPutObj_ObjStore_S3::send_response()
{
  if (ret) {
    set_req_state_err(s, ret);
  } else {
    if (s->cct->_conf->rgw_s3_success_create_obj_status) {
      ret = get_success_retcode(s->cct->_conf->rgw_s3_success_create_obj_status);
      set_req_state_err(s, ret);
    }
    dump_etag(s, etag.c_str());
    dump_content_length(s, 0);
  }
  if (s->system_request && mtime) {
    dump_epoch_header(s, "Rgwx-Mtime", mtime);
  }
  dump_errno(s);
  end_header(s, this);
}