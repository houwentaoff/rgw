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
