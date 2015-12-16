/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest_admin.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/14 16:31:40
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __PORTING_REST_ADMON_H__
#define __PORTING_REST_ADMON_H__
class RGWRESTMgr_Admin : public RGWRESTMgr {
public:
  RGWRESTMgr_Admin() {}
  virtual ~RGWRESTMgr_Admin() {}
};
#endif
