/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  cls_user_client.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 16:01:33
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __CLS_USER_CLIENT_H__
#define __CLS_USER_CLIENT_H__

#include "include/types.h"
#include "cls_user_types.h"
#include "include/rados/librados.hh"

/*
 * user objclass
 */
void cls_user_set_buckets(librados::ObjectWriteOperation& op, list<cls_user_bucket_entry>& entries, bool add);

void cls_user_bucket_list(librados::ObjectReadOperation& op,
                       const string& in_marker, int max_entries,
                       list<cls_user_bucket_entry>& entries,
                       string *out_marker, bool *truncated,
                       int *pret);
#endif
