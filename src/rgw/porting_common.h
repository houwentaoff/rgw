/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_common.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015年12月11日 13时55分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
/* ************************************ use  *********************************/
#ifndef __PORTING_COMMON_H__
#define __PORTING_COMMON_H__
/* rgw_common_porting */
#include <string>
#include <map>
#include <errno.h>
#include <string.h>
#include "common/debug.h"
#include "include/utime.h"
#include "include/types.h"
//#include "rgw_client_io.h"
#include "include/porting.h"
#include "rgw_string.h"

using namespace std;

#define RGW_ATTR_PREFIX  "user.rgw."

#define RGW_HTTP_RGWX_ATTR_PREFIX "RGWX_ATTR_"
#define RGW_HTTP_RGWX_ATTR_PREFIX_OUT "Rgwx-Attr-"

#define RGW_AMZ_META_PREFIX "x-amz-meta-"

#define RGW_SYS_PARAM_PREFIX "rgwx-"

#define RGW_ATTR_ACL		RGW_ATTR_PREFIX "acl"
#define RGW_ATTR_CORS		RGW_ATTR_PREFIX "cors"
#define RGW_ATTR_ETAG    	RGW_ATTR_PREFIX "etag"
#define RGW_ATTR_BUCKETS	RGW_ATTR_PREFIX "buckets"
#define RGW_ATTR_META_PREFIX	RGW_ATTR_PREFIX RGW_AMZ_META_PREFIX
#define RGW_ATTR_CONTENT_TYPE	RGW_ATTR_PREFIX "content_type"
#define RGW_ATTR_CACHE_CONTROL	RGW_ATTR_PREFIX "cache_control"
#define RGW_ATTR_CONTENT_DISP	RGW_ATTR_PREFIX "content_disposition"
#define RGW_ATTR_CONTENT_ENC	RGW_ATTR_PREFIX "content_encoding"
#define RGW_ATTR_CONTENT_LANG	RGW_ATTR_PREFIX "content_language"
#define RGW_ATTR_EXPIRES	RGW_ATTR_PREFIX "expires"
#define RGW_ATTR_DELETE_AT 	RGW_ATTR_PREFIX "delete_at"
#define RGW_ATTR_ID_TAG    	RGW_ATTR_PREFIX "idtag"
#define RGW_ATTR_SHADOW_OBJ    	RGW_ATTR_PREFIX "shadow_name"
#define RGW_ATTR_MANIFEST    	RGW_ATTR_PREFIX "manifest"
#define RGW_ATTR_USER_MANIFEST  RGW_ATTR_PREFIX "user_manifest"

#define RGW_ATTR_TEMPURL_KEY1   RGW_ATTR_META_PREFIX "temp-url-key"
#define RGW_ATTR_TEMPURL_KEY2   RGW_ATTR_META_PREFIX "temp-url-key-2"

#define RGW_ATTR_OLH_PREFIX     RGW_ATTR_PREFIX "olh."

#define RGW_ATTR_OLH_INFO       RGW_ATTR_OLH_PREFIX "info"
#define RGW_ATTR_OLH_VER        RGW_ATTR_OLH_PREFIX "ver"
#define RGW_ATTR_OLH_ID_TAG     RGW_ATTR_OLH_PREFIX "idtag"
#define RGW_ATTR_OLH_PENDING_PREFIX RGW_ATTR_OLH_PREFIX "pending."

#define RGW_BUCKETS_OBJ_SUFFIX ".buckets"

#define RGW_MAX_PENDING_CHUNKS  16
#define RGW_MIN_MULTIPART_SIZE (5ULL*1024*1024)

#define RGW_FORMAT_PLAIN        0
#define RGW_FORMAT_XML          1
#define RGW_FORMAT_JSON         2

#define RGW_CAP_READ            0x1
#define RGW_CAP_WRITE           0x2
#define RGW_CAP_ALL             (RGW_CAP_READ | RGW_CAP_WRITE)

#define RGW_REST_SWIFT          0x1
#define RGW_REST_SWIFT_AUTH     0x2

#define RGW_SUSPENDED_USER_AUID (uint64_t)-2

#define RGW_OP_TYPE_READ         0x01
#define RGW_OP_TYPE_WRITE        0x02
#define RGW_OP_TYPE_DELETE       0x04

#define RGW_OP_TYPE_MODIFY       (RGW_OP_TYPE_WRITE | RGW_OP_TYPE_DELETE)
#define RGW_OP_TYPE_ALL          (RGW_OP_TYPE_READ | RGW_OP_TYPE_WRITE | RGW_OP_TYPE_DELETE)

#define RGW_DEFAULT_MAX_BUCKETS 1000

#define RGW_DEFER_TO_BUCKET_ACLS_RECURSE 1
#define RGW_DEFER_TO_BUCKET_ACLS_FULL_CONTROL 2

#define STATUS_CREATED           1900
#define STATUS_ACCEPTED          1901
#define STATUS_NO_CONTENT        1902
#define STATUS_PARTIAL_CONTENT   1903
#define STATUS_REDIRECT          1904
#define STATUS_NO_APPLY          1905
#define STATUS_APPLIED           1906

#define ERR_INVALID_BUCKET_NAME  2000
#define ERR_INVALID_OBJECT_NAME  2001
#define ERR_NO_SUCH_BUCKET       2002
#define ERR_METHOD_NOT_ALLOWED   2003
#define ERR_INVALID_DIGEST       2004
#define ERR_BAD_DIGEST           2005
#define ERR_UNRESOLVABLE_EMAIL   2006
#define ERR_INVALID_PART         2007
#define ERR_INVALID_PART_ORDER   2008
#define ERR_NO_SUCH_UPLOAD       2009
#define ERR_REQUEST_TIMEOUT      2010
#define ERR_LENGTH_REQUIRED      2011
#define ERR_REQUEST_TIME_SKEWED  2012
#define ERR_BUCKET_EXISTS        2013
#define ERR_BAD_URL              2014
#define ERR_PRECONDITION_FAILED  2015
#define ERR_NOT_MODIFIED         2016
#define ERR_INVALID_UTF8         2017
#define ERR_UNPROCESSABLE_ENTITY 2018
#define ERR_TOO_LARGE            2019
#define ERR_TOO_MANY_BUCKETS     2020
#define ERR_INVALID_REQUEST      2021
#define ERR_TOO_SMALL            2022
#define ERR_NOT_FOUND            2023
#define ERR_PERMANENT_REDIRECT   2024
#define ERR_LOCKED               2025
#define ERR_QUOTA_EXCEEDED       2026
#define ERR_SIGNATURE_NO_MATCH   2027
#define ERR_INVALID_ACCESS_KEY   2028
#define ERR_MALFORMED_XML        2029
#define ERR_USER_EXIST           2030
#define ERR_USER_SUSPENDED       2100
#define ERR_INTERNAL_ERROR       2200
#define ERR_NOT_IMPLEMENTED      2201

#ifndef UINT32_MAX
#define UINT32_MAX (0xffffffffu)
#endif

typedef void *RGWAccessHandle;

enum http_op {
  OP_GET,
  OP_PUT,
  OP_DELETE,
  OP_HEAD,
  OP_POST,
  OP_COPY,
  OP_OPTIONS,
  OP_UNKNOWN,
};
class RGWConf;
class RGWEnv {
  std::map<string, string, ltstr_nocase> env_map;
public:
  RGWConf *conf; 

  RGWEnv();
  ~RGWEnv();
  void init(CephContext *cct);
  void init(CephContext *cct, char **envp);
  void set(const char *name, const char *val);
  const char *get(const char *name, const char *def_val = NULL);
  int get_int(const char *name, int def_val = 0);
  bool get_bool(const char *name, bool def_val = 0);
  size_t get_size(const char *name, size_t def_val = 0);
  bool exists(const char *name);
  bool exists_prefix(const char *prefix);

  void remove(const char *name);

  std::map<string, string, ltstr_nocase>& get_map() { return env_map; }
};

class RGWConf {
  friend class RGWEnv;
protected:
  void init(CephContext *cct, RGWEnv * env);
public:
  RGWConf() :
    enable_ops_log(1), enable_usage_log(1), defer_to_bucket_acls(0) {}

  int enable_ops_log;
  int enable_usage_log;
  uint8_t defer_to_bucket_acls;
};
/* Helper class used for RGWHTTPArgs parsing */
class NameVal
{
   string str;
   string name;
   string val;
 public:
    NameVal(string nv) : str(nv) {}

    int parse();

    string& get_name() { return name; }
    string& get_val() { return val; }
};
/** Stores the XML arguments associated with the HTTP request in req_state*/
class RGWHTTPArgs
{
  string str, empty_str;
  map<string, string> val_map;
  map<string, string> sys_val_map;
  map<string, string> sub_resources;

  bool has_resp_modifier;
 public:
  RGWHTTPArgs() : has_resp_modifier(false) {}
  /** Set the arguments; as received */
  void set(string s) {
    has_resp_modifier = false;
    val_map.clear();
    sub_resources.clear();
    str = s;
  }
  /** parse the received arguments */
  int parse();
  /** Get the value for a specific argument parameter */
  string& get(const string& name, bool *exists = NULL);
  string& get(const char *name, bool *exists = NULL);
  int get_bool(const string& name, bool *val, bool *exists);
  int get_bool(const char *name, bool *val, bool *exists);
  void get_bool(const char *name, bool *val, bool def_val);

  /** see if a parameter is contained in this RGWHTTPArgs */
  bool exists(const char *name) {
    map<string, string>::iterator iter = val_map.find(name);
    return (iter != val_map.end());
  }
  bool sub_resource_exists(const char *name) {
    map<string, string>::iterator iter = sub_resources.find(name);
    return (iter != sub_resources.end());
  }
  map<string, string>& get_params() {
    return val_map;
  }
  map<string, string>& get_sub_resources() { return sub_resources; }
  unsigned get_num_params() {
    return val_map.size();
  }
  bool has_response_modifier() {
    return has_resp_modifier;
  }
  void set_system() { /* make all system params visible */
    map<string, string>::iterator iter;
    for (iter = sys_val_map.begin(); iter != sys_val_map.end(); ++iter) {
      val_map[iter->first] = iter->second;
    }
  }
};

/* * Store error returns for output at a different point in the program */
struct rgw_err {
    rgw_err();
    rgw_err(int http, const std::string &s3);
    void clear();
    bool is_clear() const;
    bool is_err() const;
    friend std::ostream& operator<<(std::ostream& oss, const rgw_err &err);

    int http_ret;
    int ret;
    std::string s3_code;
    std::string message;
};
struct req_info {
    RGWEnv *env;
    RGWHTTPArgs args;
    map<string, string> x_meta_map;

    string host;
    const char *method;
    string script_uri;
    string request_uri;
    string effective_uri;
    string request_params;
    string domain;

    req_info(CephContext *cct, RGWEnv *_env);
    void rebuild_from(req_info& src);
    void init_meta_info(bool *found_bad_meta);
};
struct rgw_obj_key {
  string name;
  string instance;

  rgw_obj_key() {}
  rgw_obj_key(const string& n) {
    set(n);
  }
  rgw_obj_key(const string& n, const string& i) {
    set(n, i);
  }

//  rgw_obj_key(const cls_rgw_obj_key& k) {
//    set(k);
//  }

  void set(/*  const cls_rgw_obj_key& k*/) {
//    name = k.name;
//    instance = k.instance;
  }

  void transform(/*cls_rgw_obj_key *k*/) {
//    k->name = name;
//    k->instance = instance;
  }

  void set(const string& n) {
    name = n;
    instance.clear();
  }

  void set(const string& n, const string& i) {
    name = n;
    instance = i;
  }

  bool empty() {
    return name.empty();
  }
  bool operator==(const rgw_obj_key& k) const {
    return (name.compare(k.name) == 0) &&
           (instance.compare(k.instance) == 0);
  }
  bool operator<(const rgw_obj_key& k) const {
    int r = name.compare(k.name);
    if (r == 0) {
      r = instance.compare(k.instance);
    }
    return (r < 0);
  }
  bool operator<=(const rgw_obj_key& k) const {
    return !(k < *this);
  }
  void encode(bufferlist& bl) const {
    ENCODE_START(1, 1, bl);
    ::encode(name, bl);
    ::encode(instance, bl);
    ENCODE_FINISH(bl);
  }
  void decode(bufferlist::iterator& bl) {
    DECODE_START(1, bl);
    ::decode(name, bl);
    ::decode(instance, bl);
    DECODE_FINISH(bl);
  }
  void dump(Formatter *f) const;
};
WRITE_CLASS_ENCODER(rgw_obj_key)

inline ostream& operator<<(ostream& out, const rgw_obj_key &o) {
  if (o.instance.empty()) {
    return out << o.name;
  } else {
    return out << o.name << "[" << o.instance << "]";
  }
}

/* * Store all the state necessary to complete and respond to an HTTP request*/
class RGWClientIO;
struct req_state {
    CephContext *cct;
    RGWClientIO *cio;
    http_op op;
    bool content_started;
    int format;
    ceph::Formatter *formatter;
    string decoded_uri;
    string relative_uri;
    const char *length;
    int64_t content_length;
    map<string, string> generic_attrs;
    struct rgw_err err;
    bool expect_cont;
    bool header_ended;
    uint64_t obj_size;
    bool enable_ops_log;
    bool enable_usage_log;
    uint8_t defer_to_bucket_acls;
    uint32_t perm_mask;
    utime_t header_time;

//    rgw_bucket bucket;
    string bucket_name_str;
    rgw_obj_key object;
    string src_bucket_name;
//    rgw_obj_key src_object;
//    ACLOwner bucket_owner;
//    ACLOwner owner;

    string region_endpoint;
    string bucket_instance_id;

//    RGWBucketInfo bucket_info;
    map<string, bufferlist> bucket_attrs;
    bool bucket_exists;

    bool has_bad_meta;

//    RGWUserInfo user; 
//    RGWAccessControlPolicy *bucket_acl;
//    RGWAccessControlPolicy *object_acl;

    bool system_request;

    string canned_acl;
    bool has_acl_header;
    const char *copy_source;
    const char *http_auth;
    bool local_source; /*  source is local */
    
    int prot_flags;

    const char *os_auth_token;
    string swift_user;
    string swift_groups;

    utime_t time;

    void *obj_ctx;

    string dialect;

    string req_id;

    string trans_id;

    req_info info;

    req_state(CephContext *_cct, class RGWEnv *e);
    ~req_state();
};
static inline int rgw_str_to_bool(const char *s, int def_val)
{
  if (!s)
    return def_val;

  return (strcasecmp(s, "on") == 0 ||
          strcasecmp(s, "yes") == 0 ||
          strcasecmp(s, "1") == 0);
}
/** Convert an input URL into a sane object name
 * by converting %-escaped strings into characters, etc*/
extern bool url_decode(string& src_str, string& dest_str, bool in_query = false);
extern void url_encode(const string& src, string& dst);
extern int parse_key_value(string& in_str, string& key, string& val);
extern int parse_key_value(string& in_str, const char *delim, string& key, string& val);
/** time parsing */
extern int parse_time(const char *time_str, time_t *time);

#endif
