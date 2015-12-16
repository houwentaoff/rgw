/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/11 16:26:33
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include "porting_rest.h"
#include "porting_common.h"
#include "porting_op.h"
#include "common/utf8.h"

/* avoid duplicate hostnames in hostnames list */
static set<string> hostnames_set;
static map<string, string> generic_attrs_map;

int RGWHandler_ObjStore::read_permissions(RGWOp *op_obj)
{
  bool only_bucket;

  switch (s->op) {
  case OP_HEAD:
  case OP_GET:
    only_bucket = false;
    break;
  case OP_PUT:
  case OP_POST:
  case OP_COPY:
    /* is it a 'multi-object delete' request? */
    if (s->info.request_params == "delete") {
      only_bucket = true;
      break;
    }
    if (is_obj_update_op()) {
      only_bucket = false;
      break;
    }
    /* is it a 'create bucket' request? */
    if (op_obj->get_type() == RGW_OP_CREATE_BUCKET)
      return 0;
    only_bucket = true;
    break;
  case OP_DELETE:
    only_bucket = true;
    break;
  case OP_OPTIONS:
    only_bucket = true;
    break;
  default:
    return -EINVAL;
  }

  return do_read_permissions(op_obj, only_bucket);
}

// "The name for a key is a sequence of Unicode characters whose UTF-8 encoding
// is at most 1024 bytes long."
// However, we can still have control characters and other nasties in there.
// Just as long as they're utf-8 nasties.
int RGWHandler_ObjStore::validate_object_name(const string& object)
{
  int len = object.size();
  if (len > 1024) {
    // Name too long
    return -ERR_INVALID_OBJECT_NAME;
  }

  if (check_utf8(object.c_str(), len)) {
    // Object names must be valid UTF-8.
    return -ERR_INVALID_OBJECT_NAME;
  }
  return 0;
}

// This function enforces Amazon's spec for bucket names.
// (The requirements, not the recommendations.)
int RGWHandler_ObjStore::validate_bucket_name(const string& bucket)
{
  int len = bucket.size();
  if (len < 3) {
    if (len == 0) {
      // This request doesn't specify a bucket at all
      return 0;
    }
    // Name too short
    return -ERR_INVALID_BUCKET_NAME;
  }
  else if (len > 255) {
    // Name too long
    return -ERR_INVALID_BUCKET_NAME;
  }

  return 0;
}

int RGWHandler_ObjStore::allocate_formatter(struct req_state *s, int default_type, bool configurable)
{
    s->format = default_type;
    if (configurable) {
        string format_str = s->info.args.get("format");
        if (format_str.compare("xml") == 0) {
            s->format = RGW_FORMAT_XML;
        } else if (format_str.compare("json") == 0) {
            s->format = RGW_FORMAT_JSON;
        } else {
            const char *accept = s->info.env->get("HTTP_ACCEPT");
            if (accept) {
                char format_buf[64];
                unsigned int i = 0;
                for (; i < sizeof(format_buf) - 1 && accept[i] && accept[i] != ';'; ++i) {
                    format_buf[i] = accept[i];
                }
                format_buf[i] = 0;
                if ((strcmp(format_buf, "text/xml") == 0) || (strcmp(format_buf, "application/xml") == 0)) {
                    s->format = RGW_FORMAT_XML;
                } else if (strcmp(format_buf, "application/json") == 0) {
                    s->format = RGW_FORMAT_JSON;
                }
            }
        }
    }

    switch (s->format) {
        case RGW_FORMAT_PLAIN:
//            s->formatter = new RGWFormatter_Plain;
            break;
        case RGW_FORMAT_XML:
//            s->formatter = new XMLFormatter(false);
            break;
        case RGW_FORMAT_JSON:
//            s->formatter = new JSONFormatter(false);
            break;
        default:
            return -EINVAL;

    };
//    s->formatter->reset();

    return 0;
}

void RGWRESTMgr::register_resource(string resource, RGWRESTMgr *mgr)
{
    string r = "/";
    r.append(resource);

    /* do we have a resource manager registered for this entry point? */
    map<string, RGWRESTMgr *>::iterator iter = resource_mgrs.find(r);
    if (iter != resource_mgrs.end()) {
        delete iter->second;
    }
    resource_mgrs[r] = mgr;
    resources_by_size.insert(pair<size_t, string>(r.size(), r));

    /* now build default resource managers for the path (instead of nested entry points)
     * e.g., if the entry point is /auth/v1.0/ then we'd want to create a default
     * manager for /auth/
     */

    size_t pos = r.find('/', 1);

    while (pos != r.size() - 1 && pos != string::npos) {
        string s = r.substr(0, pos);

        iter = resource_mgrs.find(s);
        if (iter == resource_mgrs.end()) { /* only register it if one does not exist */
            resource_mgrs[s] = new RGWRESTMgr; /* a default do-nothing manager */
            resources_by_size.insert(pair<size_t, string>(s.size(), s));
        }

        pos = r.find('/', pos + 1);
    }
}

void RGWRESTMgr::register_default_mgr(RGWRESTMgr *mgr)
{
    delete default_mgr;
    default_mgr = mgr;
}
RGWRESTMgr *RGWRESTMgr::get_resource_mgr(struct req_state *s, const string& uri, string *out_uri)
{
    *out_uri = uri;

    if (resources_by_size.empty())
        return this;

    multimap<size_t, string>::reverse_iterator iter;

    for (iter = resources_by_size.rbegin(); iter != resources_by_size.rend(); ++iter) {
        string& resource = iter->second;
        if (uri.compare(0, iter->first, resource) == 0 &&
                (uri.size() == iter->first ||
                 uri[iter->first] == '/')) {
            string suffix = uri.substr(iter->first);
            return resource_mgrs[resource]->get_resource_mgr(s, suffix, out_uri);
        }
    }

  if (default_mgr)
    return default_mgr;

  return this;
}

RGWRESTMgr::~RGWRESTMgr()
{
  map<string, RGWRESTMgr *>::iterator iter;
  for (iter = resource_mgrs.begin(); iter != resource_mgrs.end(); ++iter) {
    delete iter->second;
  }
  delete default_mgr;
}
static bool str_ends_with(const string& s, const string& suffix, size_t *pos)
{
  size_t len = suffix.size();
  if (len > (size_t)s.size()) {
    return false;
  }

  ssize_t p = s.size() - len;
  if (pos) {
    *pos = p;
  }

  return s.compare(p, len, suffix) == 0;
}
static bool rgw_find_host_in_domains(const string& host, string *domain, string *subdomain)
{
  set<string>::iterator iter;
  for (iter = hostnames_set.begin(); iter != hostnames_set.end(); ++iter) {
    size_t pos;
    if (!str_ends_with(host, *iter, &pos))
      continue;

    if (pos == 0) {
      *domain = host;
      subdomain->clear();
    } else {
      if (host[pos - 1] != '.') {
        continue;
      }

      *domain = host.substr(pos);
      *subdomain = host.substr(0, pos - 1);
    }
    return true;
  }
  return false;
}
static int64_t parse_content_length(const char *content_length)
{
  int64_t len = -1;

  if (*content_length == '\0') {
    len = 0;
  } else {
    string err;
    len = strict_strtoll(content_length, 10, &err);
    if (!err.empty()) {
      len = -1;
    }
  }

  return len;
}

static http_op op_from_method(const char *method)
{
  if (!method)
    return OP_UNKNOWN;
  if (strcmp(method, "GET") == 0)
    return OP_GET;
  if (strcmp(method, "PUT") == 0)
    return OP_PUT;
  if (strcmp(method, "DELETE") == 0)
    return OP_DELETE;
  if (strcmp(method, "HEAD") == 0)
    return OP_HEAD;
  if (strcmp(method, "POST") == 0)
    return OP_POST;
  if (strcmp(method, "COPY") == 0)
    return OP_COPY;
  if (strcmp(method, "OPTIONS") == 0)
    return OP_OPTIONS;

  return OP_UNKNOWN;
}

int RGWREST::preprocess(struct req_state *s, RGWClientIO *cio)
{
  req_info& info = s->info;

  s->cio = cio;
  if (info.host.size()) {
    ldout(s->cct, 10) << "host=" << info.host << dendl;
    string domain;
    string subdomain;
    bool in_hosted_domain = rgw_find_host_in_domains(info.host, &domain,
						     &subdomain);
    ldout(s->cct, 20) << "subdomain=" << subdomain << " domain=" << domain
		      << " in_hosted_domain=" << in_hosted_domain << dendl;

    if (/*g_conf->rgw_resolve_cname &&*/ !in_hosted_domain) {
      string cname;
      bool found;
      int r = 0;//rgw_resolver->resolve_cname(info.host, cname, &found);
      if (r < 0) {
	ldout(s->cct, 0) << "WARNING: rgw_resolver->resolve_cname() returned r=" << r << dendl;
      }
      if (found) {
        ldout(s->cct, 5) << "resolved host cname " << info.host << " -> "
			 << cname << dendl;
        in_hosted_domain = rgw_find_host_in_domains(cname, &domain, &subdomain);
        ldout(s->cct, 20) << "subdomain=" << subdomain << " domain=" << domain
			  << " in_hosted_domain=" << in_hosted_domain << dendl;
      }
    }

    if (in_hosted_domain && !subdomain.empty()) {
      string encoded_bucket = "/";
      encoded_bucket.append(subdomain);
      if (s->info.request_uri[0] != '/')
        encoded_bucket.append("/'");
      encoded_bucket.append(s->info.request_uri);
      s->info.request_uri = encoded_bucket;
    }

    if (!domain.empty()) {
      s->info.domain = domain;
    }
  }

  if (s->info.domain.empty()) {
    s->info.domain = "test domain";//s->cct->_conf->rgw_dns_name;
  }

  url_decode(s->info.request_uri, s->decoded_uri);

  /* FastCGI specification, section 6.3
   * http://www.fastcgi.com/devkit/doc/fcgi-spec.html#S6.3
   * ===
   * The Authorizer application receives HTTP request information from the Web
   * server on the FCGI_PARAMS stream, in the same format as a Responder. The
   * Web server does not send CONTENT_LENGTH, PATH_INFO, PATH_TRANSLATED, and
   * SCRIPT_NAME headers.
   * ===
   * Ergo if we are in Authorizer role, we MUST look at HTTP_CONTENT_LENGTH
   * instead of CONTENT_LENGTH for the Content-Length.
   *
   * There is one slight wrinkle in this, and that's older versions of 
   * nginx/lighttpd/apache setting BOTH headers. As a result, we have to check
   * both headers and can't always simply pick A or B.
   */
  const char* content_length = info.env->get("CONTENT_LENGTH");
  const char* http_content_length = info.env->get("HTTP_CONTENT_LENGTH");
  if (!http_content_length != !content_length) {
    /* Easy case: one or the other is missing */
    s->length = (content_length ? content_length : http_content_length);
  } else if (/*s->cct->_conf->rgw_content_length_compat &&*/ content_length && http_content_length) {
    /* Hard case: Both are set, we have to disambiguate */
    int64_t content_length_i, http_content_length_i;

    content_length_i = parse_content_length(content_length);
    http_content_length_i = parse_content_length(http_content_length);

    // Now check them:
    if (http_content_length_i < 0) {
      // HTTP_CONTENT_LENGTH is invalid, ignore it
    } else if (content_length_i < 0) {
      // CONTENT_LENGTH is invalid, and HTTP_CONTENT_LENGTH is valid
      // Swap entries
      content_length = http_content_length;
    } else {
      // both CONTENT_LENGTH and HTTP_CONTENT_LENGTH are valid
      // Let's pick the larger size
      if (content_length_i < http_content_length_i) {
	// prefer the larger value
	content_length = http_content_length;
      }
    }
    s->length = content_length;
    // End of: else if (s->cct->_conf->rgw_content_length_compat && content_length &&
    // http_content_length)
  } else {
    /* no content length was defined */
    s->length = NULL;
  }


  if (s->length) {
    if (*s->length == '\0') {
      s->content_length = 0;
    } else {
      string err;
      s->content_length = strict_strtoll(s->length, 10, &err);
      if (!err.empty()) {
        ldout(s->cct, 10) << "bad content length, aborting" << dendl;
        return -EINVAL;
      }
    }
  }

  if (s->content_length < 0) {
    ldout(s->cct, 10) << "negative content length, aborting" << dendl;
    return -EINVAL;
  }

  map<string, string>::iterator giter;
  for (giter = generic_attrs_map.begin(); giter != generic_attrs_map.end(); ++giter) {
    const char *env = info.env->get(giter->first.c_str());
    if (env) {
      s->generic_attrs[giter->second] = env;
    }
  }

  s->http_auth = info.env->get("HTTP_AUTHORIZATION");

  if (1/*g_conf->rgw_print_continue*/) {
    const char *expect = info.env->get("HTTP_EXPECT");
    s->expect_cont = (expect && !strcasecmp(expect, "100-continue"));
  }
  s->op = op_from_method(info.method);

//  info.init_meta_info(&s->has_bad_meta);

  return 0;
}

RGWHandler *RGWREST::get_handler(RGWRados *store, struct req_state *s, RGWClientIO *cio,
				 RGWRESTMgr **pmgr, int *init_error)
{
  RGWHandler *handler;

  *init_error = preprocess(s, cio);
  if (*init_error < 0)
    return NULL;

  RGWRESTMgr *m = mgr.get_resource_mgr(s, s->decoded_uri, &s->relative_uri);
  if (!m) {
    *init_error = -ERR_METHOD_NOT_ALLOWED;
    return NULL;
  }

  if (pmgr)
    *pmgr = m;

  handler = m->get_handler(s);
  if (!handler) {
    *init_error = -ERR_METHOD_NOT_ALLOWED;
    return NULL;
  }
  *init_error = handler->init(store, s, cio);
  if (*init_error < 0) {
    m->put_handler(handler);
    return NULL;
  }

  return handler;
}


