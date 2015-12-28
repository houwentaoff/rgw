// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include/types.h"
#include "cls/rgw/cls_rgw_ops.h"
#include "cls/rgw/cls_rgw_client.h"
#include "include/rados/librados.hh"

#include "common/debug.h"
#include "common/shell.h"

#include "global/global.h"

using namespace librados;

static bool issue_bucket_list_op(librados::IoCtx& io_ctx,
    const string& oid, const cls_rgw_obj_key& start_obj, const string& filter_prefix,
    uint32_t num_entries, bool list_versions, BucketIndexAioManager *manager,
    struct rgw_cls_list_ret *pdata) {
  bufferlist in;
  struct rgw_cls_list_op call;
  call.start_obj = start_obj;
  call.filter_prefix = filter_prefix;
  call.num_entries = num_entries;
  call.list_versions = list_versions;
  ::encode(call, in);

  librados::ObjectReadOperation op;
  /* :TODO:2015/12/28 16:25:14:hwt:  */
  struct rgw_bucket_dir_entry test;
  //1. get bucket name from oid.
  char bucket_name[256] = {0};  
  char cmd_buf[512] = {0};
  
  sscanf(oid.c_str(), ".dir.%s", bucket_name);
  sprintf(cmd_buf, "ls %s/%s", G.buckets_root.c_str(), bucket_name);
  string files = shell_execute(cmd_buf);
  if (files == "")
  {
    return true;
  }
  const char *pos = files.c_str();
  char file_name[256]={0};
  int size = 0;
  struct stat st;
  string base_path = G.buckets_root + string("/") + string(bucket_name) + string("/");
  string full_path;

  for (; *pos!='\0'; pos += size)
  {
    int r = sscanf(pos, "%s", file_name);
    if (r!=1)
      break;
    size = strlen(file_name)+1;
    test.key.name  = file_name;
    full_path = base_path + string(file_name);
    if (stat(full_path.c_str(), &st) < 0)
    {
      continue;
    }
    utime_t  utime(st.st_mtime, 0);
    test.meta.size  =  st.st_size;
    test.meta.mtime = utime;
    pdata->dir.m[file_name] = test;
  }
   /* :TODO:End---  */
//  op.exec("rgw", "bucket_list", in, new ClsBucketIndexOpCtx<struct rgw_cls_list_ret>(pdata, NULL));
  return true;//manager->aio_operate(io_ctx, oid, &op);
}

int CLSRGWIssueBucketList::issue_op(int shard_id, const string& oid)
{
  return issue_bucket_list_op(io_ctx, oid, start_obj, filter_prefix, num_entries, list_versions, NULL/*  &manager */, &result[shard_id]);
}


