// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#include <errno.h>

#include "include/types.h"
#include "cls/rgw/cls_rgw_ops.h"
#include "cls/rgw/cls_rgw_client.h"
#include "include/rados/librados.hh"

#include "common/debug.h"

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
//  op.exec("rgw", "bucket_list", in, new ClsBucketIndexOpCtx<struct rgw_cls_list_ret>(pdata, NULL));
  return true;//manager->aio_operate(io_ctx, oid, &op);
}

int CLSRGWIssueBucketList::issue_op(int shard_id, const string& oid)
{
  return issue_bucket_list_op(io_ctx, oid, start_obj, filter_prefix, num_entries, list_versions, NULL/*  &manager */, &result[shard_id]);
}


