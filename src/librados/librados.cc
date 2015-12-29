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
void librados::ObjectReadOperation::stat(uint64_t *psize, time_t *pmtime, int *prval)
{
 /* :TODO:12/29/2015 11:48:54 PM:hwt:  */
    struct stat st;
    int ret = -1;

    if ((ret = ::stat("/path", &st)) < 0)
    {
        goto done;
    }
    *psize   = st.st_size;
    *pmtime = st.st_mtime;

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
    int64_t left = len;
    int ret = -1;
    int fd = -1;

    if ((fd = ::open("/path", O_RDONLY)) < 0)
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
