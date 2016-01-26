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

#include "librados.h"

#if __GNUC__ >= 4
  #define CEPH_RADOS_API  __attribute__ ((visibility ("default")))
#else
  #define CEPH_RADOS_API
#endif

namespace librados
{
  class CEPH_RADOS_API IoCtx;
  class RadosClient;
  class CEPH_RADOS_API Rados
  {
      public:
          int pool_create(const char *name);
          int pool_create(const char *name, uint64_t auid);
          int pool_create(const char *name, uint64_t auid, uint8_t crush_rule);
          int mon_command(std::string cmd, /*const*/ bufferlist& inbl,
		                  bufferlist *outbl, std::string *outs);
          static void version(int *major, int *minor, int *extra);

          Rados();
          explicit Rados(IoCtx& ioctx);
          ~Rados();
       private:
        // We don't allow assignment or copying
        Rados(const Rados& rhs);
        const Rados& operator=(const Rados& rhs);
        RadosClient *client;          
  };

  class ObjectOperationImpl;  
  /*
   * ObjectOperation : compound object operation
   * Batch multiple object operations into a single request, to be applied
   * atomically.
   */
  class CEPH_RADOS_API ObjectOperation
  {
    string oid;
    string full_path;
    uint64_t size;
  public:
    ObjectOperation(const char * _oid, const char * _full_path);
    ObjectOperation(){};
    virtual ~ObjectOperation(){};
    void exec(const char *cls, const char *method, bufferlist& inbl){};
    void exec(const char *cls, const char *method, bufferlist& inbl, bufferlist *obl, int *prval){};
    string get_oid(){return oid;}
    string get_path(){return full_path;}
    uint64_t get_size(){return size;}
    void set_size(uint64_t _size){size = _size;}
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
        ObjectReadOperation(const char * _oid, const char * _full_path):ObjectOperation(_oid, _full_path) {}
        ObjectReadOperation() {}
        ~ObjectReadOperation() {}
    void stat(uint64_t *psize, time_t *pmtime, int *prval);
    void getxattr(const char *name, bufferlist *pbl, int *prval){};
    void getxattrs(std::map<std::string, bufferlist> *pattrs, int *prval){};
    void read(size_t off, uint64_t len, bufferlist *pbl, int *prval);
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
  typedef void *completion_t;
  typedef void (*callback_t)(completion_t cb, void *arg);

  struct CEPH_RADOS_API AioCompletion {
//    AioCompletion(AioCompletionImpl *pc_) : pc(pc_) {}
    int set_complete_callback(void *cb_arg, callback_t cb){};
    int set_safe_callback(void *cb_arg, callback_t cb){};
    int wait_for_complete(){};
    int wait_for_safe(){};
    int wait_for_complete_and_cb(){};
    int wait_for_safe_and_cb(){};
    bool is_complete(){};
    bool is_safe(){};
    bool is_complete_and_cb(){};
    bool is_safe_and_cb(){};
    int get_return_value(){};
    int get_version() __attribute__ ((deprecated)){};
    uint64_t get_version64(){};
    void release(){};
//    AioCompletionImpl *pc;
  };  
  /*
   * ObjectWriteOperation : compound object write operation
   * Batch multiple object operations into a single request, to be applied
   * atomically.
   */
  class CEPH_RADOS_API ObjectWriteOperation : public ObjectOperation
  { 
  protected:
    time_t *pmtime;
  public:
    ObjectWriteOperation() : pmtime(NULL) {}
    ~ObjectWriteOperation() {}

    void mtime(time_t *pt) {
      pmtime = pt;
    }
    

    friend class IoCtx;
  };
}
#endif
