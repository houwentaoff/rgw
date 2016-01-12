// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <errno.h>

#include <string>
#include <map>

#include "common/errno.h"
#include "common/Formatter.h"
#include "common/ceph_json.h"
#include "common/RWLock.h"
//#include "rgw_rados.h"
#include "porting_acl.h"

#include "include/types.h"
#include "porting_user.h"
#include "rgw_string.h"

// until everything is moved from rgw_common
#include "porting_common.h"

#include "porting_bucket.h"
#include "global/global.h"

#define dout_subsys ceph_subsys_rgw

using namespace std;


//static RGWMetadataHandler *user_meta_handler = NULL;


/**
 * Get the anonymous (ie, unauthenticated) user info.
 */
void rgw_get_anon_user(RGWUserInfo& info)
{
  info.user_id = RGW_USER_ANON_ID;
  info.display_name.clear();
  info.access_keys.clear();
}

bool rgw_user_is_authenticated(RGWUserInfo& info)
{
  return (info.user_id != RGW_USER_ANON_ID);
}

/**
 * Given an access key, finds the user info associated with it.
 * returns: 0 on success, -ERR# on failure (including nonexistence)
 */
extern int rgw_get_user_info_by_access_key(RGWRados *store, string& access_key, RGWUserInfo& info,
                                           RGWObjVersionTracker *objv_tracker, time_t *pmtime)
{
  int uid;

#ifdef FICS
  string fics_id = string("_fics_") + access_key;
  uid = getuid(fics_id.c_str());
#else
  uid = getuid(access_key.c_str());
#endif
  if (uid < 0)
  {
    return -1;
  }
  info.auid = uid;
  info.display_name = access_key;
  return 0;//rgw_get_user_info_from_index(store, access_key, store->zone.user_keys_pool, info, objv_tracker, pmtime);
}

