/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_json_enc.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  Monday, December 21, 2015 10:43:21 CST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include "porting_common.h"
#include "common/ceph_json.h"

void RGWBucketEnt::dump(Formatter *f) const
{
#if 0
  encode_json("bucket", bucket, f);
  encode_json("size", size, f);
  encode_json("size_rounded", size_rounded, f);
  encode_json("mtime", creation_time, f); /* mtime / creation time discrepency needed for backward compatibility */
  encode_json("count", count, f);
#endif
}

