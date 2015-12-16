/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest_bucket.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/14 16:53:31
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include "porting_op.h"
//#include "porting_bucket.h"
#include "porting_rest_bucket.h"

RGWOp *RGWHandler_Bucket::op_get()
{
    return NULL;
}
RGWOp *RGWHandler_Bucket::op_put()
{
  return  NULL ;//new;
}
RGWOp *RGWHandler_Bucket::op_post()
{
  return  NULL;//new;
}
RGWOp *RGWHandler_Bucket::op_delete()
{
  return NULL;//new;
}
