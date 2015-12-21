/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  librados.hh
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 15:27:10
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#ifndef __LIBRADOS_HH__
#define __LIBRADOS_HH__

#include <stdbool.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include "memory.h"
#include "buffer.h"

#if __GNUC__ >= 4
  #define CEPH_RADOS_API  __attribute__ ((visibility ("default")))
#else
  #define CEPH_RADOS_API
#endif

namespace librados
{
  class ObjectOperationImpl;  
  /*
   * ObjectOperation : compound object operation
   * Batch multiple object operations into a single request, to be applied
   * atomically.
   */
  class CEPH_RADOS_API ObjectOperation
  {
  public:
    ObjectOperation();
    virtual ~ObjectOperation();
    void exec(const char *cls, const char *method, bufferlist& inbl);
    void exec(const char *cls, const char *method, bufferlist& inbl, bufferlist *obl, int *prval);
  protected:
//    ObjectOperationImpl *impl;
    ObjectOperation(const ObjectOperation& rhs);
    ObjectOperation& operator=(const ObjectOperation& rhs);
    
  }; 
}
#endif
