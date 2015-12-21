/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rados.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 15:14:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "porting_rados.h"

int RGWRados::cls_user_list_buckets(rgw_obj& obj,
                                    const string& in_marker, int max_entries,
                                    list<cls_user_bucket_entry>& entries,
                                    string *out_marker, bool *truncated)
{
  librados::ObjectReadOperation op;
  int rc;

  cls_user_bucket_list(op, in_marker, max_entries, entries, out_marker, truncated, &rc);
  if (rc < 0)
    return rc;    

    return 0;
}

