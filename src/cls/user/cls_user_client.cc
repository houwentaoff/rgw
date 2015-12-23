/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  cls_user_client.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 16:03:38
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "cls_user_client.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "common/shell.h"


void cls_user_bucket_list(librados::ObjectReadOperation& op,
                       const string& in_marker, int max_entries, list<cls_user_bucket_entry>& entries,
                       string *out_marker, bool *truncated, int *pret)
{
#if 0
  bufferlist inbl;
  cls_user_list_buckets_op call;
  call.marker = in_marker;
  call.max_entries = max_entries;

  ::encode(call, inbl);

  op.exec("user", "list_buckets", inbl, new ClsUserListCtx(&entries, out_marker, truncated, pret));
#endif
  cls_user_bucket_entry tmp;
  vector<string> fileVec;
  string files = shell_execute("ls /fisamba");
  if (files == "")
  {
      return;
  }
  boost::split(fileVec, files, boost::is_any_of("\n"), boost::token_compress_on );
  for (vector<string>::iterator it = fileVec.begin(); it!=fileVec.end(); it++)
  {
      tmp.bucket.name = *it;
      entries.push_back(tmp);
  }
  *pret = 0;
//  *truncated = true;
}

