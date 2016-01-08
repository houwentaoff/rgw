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
#include "auth/Crypto.h"
#include "common/ceph_crypto.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/fsuid.h>
#include "global/global.h"

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
bool verify_bucket_permission(struct req_state *s, int perm)
{
#if 0
  if (!s->bucket_acl)
    return false;

  if ((perm & (int)s->perm_mask) != perm)
    return false;

  if (!verify_requester_payer_permission(s))
    return false;

  return s->bucket_acl->verify_permission(s->user.user_id, perm, perm);    
#endif
  return true;
}

static bool char_needs_url_encoding(char c)
{
  if (c <= 0x20 || c >= 0x7f)
    return true;

  switch (c) {
    case 0x22:
    case 0x23:
    case 0x25:
    case 0x26:
    case 0x2B:
    case 0x2C:
    case 0x2F:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3E:
    case 0x3D:
    case 0x3F:
    case 0x40:
    case 0x5B:
    case 0x5D:
    case 0x5C:
    case 0x5E:
    case 0x60:
    case 0x7B:
    case 0x7D:
      return true;
  }
  return false;
}
static void escape_char(char c, string& dst)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%%%.2X", (int)(unsigned char)c);
  dst.append(buf);
}

void url_encode(const string& src, string& dst)
{
  const char *p = src.c_str();
  for (unsigned i = 0; i < src.size(); i++, p++) {
    if (char_needs_url_encoding(*p)) {
      escape_char(*p, dst);
      continue;
    }

    dst.append(p, 1);
  }
}
static bool check_gmt_end(const char *s)
{
  if (!s || !*s)
    return false;

  while (isspace(*s)) {
    ++s;
  }

  /* check for correct timezone */
  if ((strncmp(s, "GMT", 3) != 0) &&
      (strncmp(s, "UTC", 3) != 0)) {
    return false;
  }

  return true;
}
static bool parse_rfc850(const char *s, struct tm *t)
{
  memset(t, 0, sizeof(*t));
  return check_gmt_end(strptime(s, "%A, %d-%b-%y %H:%M:%S ", t));
}
static bool check_str_end(const char *s)
{
  if (!s)
    return false;

  while (*s) {
    if (!isspace(*s))
      return false;
    s++;
  }
  return true;
}
static bool parse_asctime(const char *s, struct tm *t)
{
  memset(t, 0, sizeof(*t));
  return check_str_end(strptime(s, "%a %b %d %H:%M:%S %Y", t));
}
static bool parse_rfc1123(const char *s, struct tm *t)
{
  memset(t, 0, sizeof(*t));
  return check_gmt_end(strptime(s, "%a, %d %b %Y %H:%M:%S ", t));
}

static bool parse_rfc1123_alt(const char *s, struct tm *t)
{
  memset(t, 0, sizeof(*t));
  return check_str_end(strptime(s, "%a, %d %b %Y %H:%M:%S %z", t));
}

bool parse_rfc2616(const char *s, struct tm *t)
{
  return parse_rfc850(s, t) || parse_asctime(s, t) || parse_rfc1123(s, t) || parse_rfc1123_alt(s,t);
}

int parse_time(const char *time_str, time_t *time)
{
  struct tm tm;

  if (!parse_rfc2616(time_str, &tm))
    return -EINVAL;

  *time = timegm(&tm);

  return 0;
}
string rgw_string_unquote(const string& s)
{
  if (s[0] != '"' || s.size() < 2)
    return s;

  int len;
  for (len = s.size(); len > 2; --len) {
    if (s[len - 1] != ' ')
      break;
  }

  if (s[len-1] != '"')
    return s;

  return s.substr(1, len - 2);
}
// this is basically a modified base64 charset, url friendly
static const char alphanum_table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int gen_rand_alphanumeric(CephContext *cct, char *dest, int size) /* size should be the required string size + 1 */
{
  int ret = get_random_bytes(dest, size);
  if (ret < 0) {
    lderr(cct) << "cannot get random bytes: " /*<< cpp_strerror(-ret) */<< dendl;
    return ret;
  }

  int i;
  for (i=0; i<size - 1; i++) {
    int pos = (unsigned)dest[i];
    dest[i] = alphanum_table[pos & 63];
  }
  dest[i] = '\0';

  return 0;
}
#define FIGROUP		"_fics_"
#if 0
{
    if (G.use_uid)
    {
        if (-1 == (new_uid = getuid(name)))
        {
            return -1;
        }
        drop_privs(new_uid);
    }
}
#endif
int getuid(const char *suffix_name)
{
    struct passwd *pwd;
    string name;
    if (string(suffix_name) == "")
    {
        name = G.user_name;
    }
    else
    {
        name = string(FIGROUP) + suffix_name;
    }
    if (NULL == (pwd = getpwnam(name.c_str())))
    {
        return -1;
    }
    return pwd->pw_uid;
}
int drop_privs(uid_t new_uid)
{
    return setfsuid(new_uid);
}
int restore_privs(uid_t old_uid)
{
    return setfsuid(old_uid);
}

/*
 * calculate the sha1 value of a given msg and key
 */
void calc_hmac_sha1(const char *key, int key_len,
                    const char *msg, int msg_len, char *dest)
/* destination should be CEPH_CRYPTO_HMACSHA1_DIGESTSIZE bytes long */
{
  using namespace ceph::crypto;
  HMACSHA1 hmac((const unsigned char *)key, key_len);
  hmac.Update((const unsigned char *)msg, msg_len);
  hmac.Final((unsigned char *)dest);
}
