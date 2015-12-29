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
    ObjectOperation(){};
    virtual ~ObjectOperation(){};
    void exec(const char *cls, const char *method, bufferlist& inbl){};
    void exec(const char *cls, const char *method, bufferlist& inbl, bufferlist *obl, int *prval){};
  protected:
//    ObjectOperationImpl *impl;
    ObjectOperation(const ObjectOperation& rhs){};
    ObjectOperation& operator=(const ObjectOperation& rhs){};
    
  }; 
  /*
   * ObjectReadOperation : compound object operation that return value
   * Batch multiple object operations into a single request, to be applied
   * atomically.
   */
  class CEPH_RADOS_API ObjectReadOperation : public ObjectOperation
  {
    public:
        ObjectReadOperation() {}
        ~ObjectReadOperation() {}
    void stat(uint64_t *psize, time_t *pmtime, int *prval){};
    void getxattr(const char *name, bufferlist *pbl, int *prval){};
    void getxattrs(std::map<std::string, bufferlist> *pattrs, int *prval){};
    void read(size_t off, uint64_t len, bufferlist *pbl, int *prval){};
  };
  /* IoCtx : This is a context in which we can perform I/O.
   * It includes a Pool,
   *
   * Typical use (error checking omitted):
   *
   * IoCtx p;
   * rados.ioctx_create("my_pool", p);
   * p->stat(&stats);
   * ... etc ...
   *
   * NOTE: be sure to call watch_flush() prior to destroying any IoCtx
   * that is used for watch events to ensure that racing callbacks
   * have completed.
   */
  class CEPH_RADOS_API IoCtx
  {
  public:
    IoCtx(){};
//    static void from_rados_ioctx_t(rados_ioctx_t p, IoCtx &pool){};
    IoCtx(const IoCtx& rhs){};
    IoCtx& operator=(const IoCtx& rhs);

    ~IoCtx(){};

    // Close our pool handle
    void close(){};

    // deep copy
    void dup(const IoCtx& rhs){};
  };  
}
#endif
