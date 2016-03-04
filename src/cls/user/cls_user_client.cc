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
#include "global/global.h"


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
  char cmd_buf[512] = {0};
  sprintf(cmd_buf, "ls %s", G.buckets_root.c_str());
  string files = shell_execute(cmd_buf);
  if (files == "")
  {
      return;
  }
  boost::split(fileVec, files, boost::is_any_of("\n"), boost::token_compress_on );
  for (vector<string>::iterator it = fileVec.begin(); it!=fileVec.end(); it++)
  {
      tmp.bucket.name = *it;
      //time_t creation_time;
      struct stat statbuf;
      stat(G.buckets_root.c_str(), &statbuf);
      tmp.creation_time   = statbuf.st_ctime;
      entries.push_back(tmp);
  }
  *pret = 0;
//  *truncated = true;
}
void cls_user_set_buckets(librados::ObjectWriteOperation& op, list<cls_user_bucket_entry>& entries, bool add)
{
#if 0
  bufferlist in;
  cls_user_set_buckets_op call;
  call.entries = entries;
  call.add = add;
  call.time = ceph_clock_now(NULL);
  ::encode(call, in);
  op.exec("user", "set_buckets_info", in);
#else

  /*-----------------------------------------------------------------------------
   *  不能修改文件夹的时间？
   *-----------------------------------------------------------------------------*/
  char cmd_buf[512] = {0};
  int ret;
  list<cls_user_bucket_entry>::iterator it;

  sprintf(cmd_buf, "mkdir %s", G.buckets_root.c_str());
  for (it = entries.begin(); it!=entries.end(); it++)
  {
#ifdef FICS
#else    
    if (true == add)
    {

        sprintf(cmd_buf, "mkdir -m 777 %s/%s", G.buckets_root.c_str(), it->bucket.name.c_str());
  
    }
    else
    {
//        sprintf(cmd_buf, "mkdir -p -m %s/%s", G.buckets_root.c_str(), it->bucket.name);
    }
    ret = shell_simple(cmd_buf);
#endif     
  }
#endif
}
