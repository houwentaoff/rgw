/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  librados.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/21 15:36:21
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include <limits.h>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include/types.h"
#include "include/rados/librados.hh"
#include "RadosClient.h"
#if 0
librados::ObjectOperation::ObjectOperation()
{
//  impl = (ObjectOperationImpl *)new ::ObjectOperation;
}

librados::ObjectOperation::~ObjectOperation()
{
//  ::ObjectOperation *o = (::ObjectOperation *)impl;
//  if (o)
//    delete o;
}

void librados::ObjectOperation::exec(const char *cls, const char *method, bufferlist& inbl)
{
//  ::ObjectOperation *o = (::ObjectOperation *)impl;
//  o->call(cls, method, inbl);
}

void librados::ObjectOperation::exec(const char *cls, const char *method, bufferlist& inbl, bufferlist *outbl, int *prval)
{
//  ::ObjectOperation *o = (::ObjectOperation *)impl;
//  o->call(cls, method, inbl, outbl, NULL, prval);
}
#endif
librados::ObjectOperation::ObjectOperation(const char * _oid = NULL, const char * _full_path = NULL)
{
    oid        = (!_oid) ? "" : _oid;
    full_path  = (!_full_path) ? "" : _full_path;
}
void librados::ObjectReadOperation::stat(uint64_t *psize, time_t *pmtime, int *prval)
{
 /* :TODO:12/29/2015 11:48:54 PM:hwt:  */
    struct stat st;
    int ret = -1;
    string path;
    string oid;

    if ((path = ObjectOperation::get_path()) != "")
    {

    }
    else
    {
        oid  = ObjectOperation::get_oid();
        //change oid to path
//        path = "";
    }
    if ((ret = ::stat(path.c_str(), &st)) < 0)
    {
        goto done;
    }
    *psize   = st.st_size;
    *pmtime  = st.st_mtime;
    ObjectOperation::set_size(st.st_size);

done:
    if (prval)
    {
        *prval = ret;
    }
 /* :TODO:End---  */
//      ::ObjectOpertion *o = (::ObjectOperation *)impl;
//        o->stat(psize, pmtime, prval);
}

void librados::ObjectReadOperation::read(size_t off, uint64_t len, bufferlist *pbl, int *prval)
{
//      ::ObjectOperation *o = (::ObjectOperation *)impl;
//        o->read(off, len, pbl, prval, NULL);
     /* :TODO:12/29/2015 11:50:08 PM:hwt:  */
#define BLOCK_SIZE          1024            /*  */
    char buf[BLOCK_SIZE+1];
    int64_t left = len > ObjectOperation::get_size() ? ObjectOperation::get_size() : len;
    int ret = -1;
    int fd = -1;
    string path;
    string oid;

    if ((path = ObjectOperation::get_path()) != "")
    {

    }
    else
    {
        oid  = ObjectOperation::get_oid();
        //change oid to path
//        path = "";
    }
    if ((fd = ::open(path.c_str(), O_RDONLY)) < 0)
    {
        ret = -1;
        goto done;
    }
    while (left > 0)
    {
        if ((ret = ::pread(fd, buf, BLOCK_SIZE, off)) < 0)
        {
            goto done;
        }
        pbl->append(buf, BLOCK_SIZE);
        left -= BLOCK_SIZE;
    }

done:
    if (prval)
    {
        *prval = ret;
    }
    if (fd > 0)
    {
        ::close(fd);
    }
     /* :TODO:End---  */
}
int librados::Rados::pool_create(const char *name)
{
  string str(name);
  return client->pool_create(str);
}

int librados::Rados::pool_create(const char *name, uint64_t auid)
{
  string str(name);
  return client->pool_create(str, auid);
}

int librados::Rados::pool_create(const char *name, uint64_t auid, __u8 crush_rule)
{
  string str(name);
  return client->pool_create(str, auid, crush_rule);
}

int librados::Rados::mon_command(string cmd, /*const*/ bufferlist& inbl,
				 bufferlist *outbl, string *outs)
{
  vector<string> cmdvec;
  cmdvec.push_back(cmd);
  return client->mon_command(cmdvec, inbl, outbl, outs);
}

librados::Rados::Rados() : client(NULL)
{
}

librados::Rados::Rados(IoCtx &ioctx)
{
//      client = ioctx.io_ctx_impl->client;
//      assert(client != NULL);
//      client->get();
}

librados::Rados::~Rados()
{
//      shutdown();
}
