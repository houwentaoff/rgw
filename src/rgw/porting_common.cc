/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_common.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/15 11:01:43
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "porting_common.h"
class HexTable
{
  char table[256];

public:
  HexTable() {
    memset(table, -1, sizeof(table));
    int i;
    for (i = '0'; i<='9'; i++)
      table[i] = i - '0';
    for (i = 'A'; i<='F'; i++)
      table[i] = i - 'A' + 0xa;
    for (i = 'a'; i<='f'; i++)
      table[i] = i - 'a' + 0xa;
  }

  char to_num(char c) {
    return table[(int)c];
  }
};
static void trim_whitespace(const string& src, string& dst)
{
  const char *spacestr = " \t\n\r\f\v";
  int start = src.find_first_not_of(spacestr);
  if (start < 0)
    return;

  int end = src.find_last_not_of(spacestr);
  dst = src.substr(start, end - start + 1);
}
int parse_key_value(string& in_str, const char *delim, string& key, string& val)
{
  if (delim == NULL)
    return -EINVAL;

  int pos = in_str.find(delim);
  if (pos < 0)
    return -EINVAL;

  trim_whitespace(in_str.substr(0, pos), key);
  pos++;

  trim_whitespace(in_str.substr(pos), val);

  return 0;
}

int parse_key_value(string& in_str, string& key, string& val)
{
  return parse_key_value(in_str, "=", key,val);
}

static char hex_to_num(char c)
{
  static HexTable hex_table;
  return hex_table.to_num(c);
}

bool url_decode(string& src_str, string& dest_str, bool in_query)
{
  const char *src = src_str.c_str();
  char dest[src_str.size() + 1];
  int pos = 0;
  char c;

  while (*src) {
    if (*src != '%') {
      if (!in_query || *src != '+') {
        if (*src == '?') in_query = true;
        dest[pos++] = *src++;
      } else {
        dest[pos++] = ' ';
        ++src;
      }
    } else {
      src++;
      if (!*src)
        break;
      char c1 = hex_to_num(*src++);
      if (!*src)
        break;
      c = c1 << 4;
      if (c1 < 0)
        return false;
      c1 = hex_to_num(*src++);
      if (c1 < 0)
        return false;
      c |= c1;
      dest[pos++] = c;
    }
  }
  dest[pos] = 0;
  dest_str = dest;

  return true;
}

string& RGWHTTPArgs::get(const string& name, bool *exists)
{
  map<string, string>::iterator iter;
  iter = val_map.find(name);
  bool e = (iter != val_map.end());
  if (exists)
    *exists = e;
  if (e)
    return iter->second;
  return empty_str;
}

string& RGWHTTPArgs::get(const char *name, bool *exists)
{
  string s(name);
  return get(s, exists);
}


int RGWHTTPArgs::get_bool(const string& name, bool *val, bool *exists)
{
  map<string, string>::iterator iter;
  iter = val_map.find(name);
  bool e = (iter != val_map.end());
  if (exists)
    *exists = e;

  if (e) {
    const char *s = iter->second.c_str();

    if (strcasecmp(s, "false") == 0) {
      *val = false;
    } else if (strcasecmp(s, "true") == 0) {
      *val = true;
    } else {
      return -EINVAL;
    }
  }

  return 0;
}
int NameVal::parse()
{
  int delim_pos = str.find('=');
  int ret = 0;

  if (delim_pos < 0) {
    name = str;
    val = "";
    ret = 1;
  } else {
    name = str.substr(0, delim_pos);
    val = str.substr(delim_pos + 1);
  }

  return ret; 
}
int RGWHTTPArgs::parse()
{
  int pos = 0;
  bool end = false;
  bool admin_subresource_added = false; 
  if (str[pos] == '?') pos++;

  while (!end) {
    int fpos = str.find('&', pos);
    if (fpos  < pos) {
       end = true;
       fpos = str.size(); 
    }
    string substr, nameval;
    substr = str.substr(pos, fpos - pos);
    url_decode(substr, nameval, true);
    NameVal nv(nameval);
    int ret = nv.parse();
    if (ret >= 0) {
      string& name = nv.get_name();
      string& val = nv.get_val();

      if (name.compare(0, sizeof(RGW_SYS_PARAM_PREFIX) - 1, RGW_SYS_PARAM_PREFIX) == 0) {
        sys_val_map[name] = val;
      } else {
        val_map[name] = val;
      }

      if ((name.compare("acl") == 0) ||
          (name.compare("cors") == 0) ||
          (name.compare("location") == 0) ||
          (name.compare("logging") == 0) ||
          (name.compare("delete") == 0) ||
          (name.compare("uploads") == 0) ||
          (name.compare("partNumber") == 0) ||
          (name.compare("uploadId") == 0) ||
          (name.compare("versionId") == 0) ||
          (name.compare("versions") == 0) ||
          (name.compare("versioning") == 0) ||
          (name.compare("requestPayment") == 0) ||
          (name.compare("torrent") == 0)) {
        sub_resources[name] = val;
      } else if (name[0] == 'r') { // root of all evil
        if ((name.compare("response-content-type") == 0) ||
           (name.compare("response-content-language") == 0) ||
           (name.compare("response-expires") == 0) ||
           (name.compare("response-cache-control") == 0) ||
           (name.compare("response-content-disposition") == 0) ||
           (name.compare("response-content-encoding") == 0)) {
          sub_resources[name] = val;
          has_resp_modifier = true;
        }
      } else if  ((name.compare("subuser") == 0) ||
          (name.compare("key") == 0) ||
          (name.compare("caps") == 0) ||
          (name.compare("index") == 0) ||
          (name.compare("policy") == 0) ||
          (name.compare("quota") == 0) ||
          (name.compare("object") == 0)) {

        if (!admin_subresource_added) {
          sub_resources[name] = "";
          admin_subresource_added = true;
        }
      }
    }

    pos = fpos + 1;  
  }

  return 0;
}

int RGWHTTPArgs::get_bool(const char *name, bool *val, bool *exists)
{
  string s(name);
  return get_bool(s, val, exists);
}

void RGWHTTPArgs::get_bool(const char *name, bool *val, bool def_val)
{
  bool exists = false;
  if ((get_bool(name, val, &exists) < 0) ||
      !exists) {
    *val = def_val;
  }
}
req_state::req_state(CephContext *_cct, class RGWEnv *e) : cct(_cct), cio(NULL), op(OP_UNKNOWN),
has_acl_header(false),
os_auth_token(NULL), info(_cct, e)
{
  enable_ops_log = e->conf->enable_ops_log;
  enable_usage_log = e->conf->enable_usage_log;
  defer_to_bucket_acls = e->conf->defer_to_bucket_acls;
  content_started = false;
  format = 0;
  formatter = NULL;
//  bucket_acl = NULL;
//  object_acl = NULL;
  expect_cont = false;

  header_ended = false;
  obj_size = 0;
  prot_flags = 0;

  system_request = false;

  os_auth_token = NULL;
//  time = ceph_clock_now(cct);
  perm_mask = 0;
  content_length = 0;
  bucket_exists = false;
  has_bad_meta = false;
  length = NULL;
  copy_source = NULL;
  http_auth = NULL;
  local_source = false;

  obj_ctx = NULL;
}

req_state::~req_state() {
  delete formatter;
//  delete bucket_acl;
//  delete object_acl;
}
req_info::req_info(CephContext *cct, class RGWEnv *e) : env(e) {
  method = env->get("REQUEST_METHOD", "");
  script_uri = env->get("SCRIPT_URI", "testSCRIPT_URI");//cct->_conf->rgw_script_uri.c_str());
  request_uri = env->get("REQUEST_URI", "test REQUEST_URI");//cct->_conf->rgw_request_uri.c_str());
  int pos = request_uri.find('?');
  if (pos >= 0) {
    request_params = request_uri.substr(pos + 1);
    request_uri = request_uri.substr(0, pos);
  } else {
    request_params = env->get("QUERY_STRING", "");
  }
  host = env->get("HTTP_HOST", "");

  // strip off any trailing :port from host (added by CrossFTP and maybe others)
  size_t colon_offset = host.find_last_of(':');
  if (colon_offset != string::npos) {
    bool all_digits = true;
    for (unsigned i = colon_offset + 1; i < host.size(); ++i) {
      if (!isdigit(host[i])) {
	all_digits = false;
	break;
      }
    }
    if (all_digits) {
      host.resize(colon_offset);
    }
  }
}

void req_info::rebuild_from(req_info& src)
{
  method = src.method;
  script_uri = src.script_uri;
  if (src.effective_uri.empty()) {
    request_uri = src.request_uri;
  } else {
    request_uri = src.effective_uri;
  }
  effective_uri.clear();
  host = src.host;

  x_meta_map = src.x_meta_map;
  x_meta_map.erase("x-amz-date");
}

rgw_err::
rgw_err()
{
  clear();
}

rgw_err::rgw_err(int http, const std::string& s3)
    : http_ret(http), ret(0), s3_code(s3)
{
}

void rgw_err::clear()
{
  http_ret = 200;
  ret = 0;
  s3_code.clear();
}

bool rgw_err::is_clear() const
{
  return (http_ret == 200);
}

bool rgw_err::is_err() const
{
  return !(http_ret >= 200 && http_ret <= 399);
}

